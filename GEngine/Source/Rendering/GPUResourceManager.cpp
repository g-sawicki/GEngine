#include "PCH.hpp"

#include "Rendering/GPUResourceManager.hpp"

#include "Core/Utility/Image.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Rendering/MeshBuffer.hpp"

namespace GEngine {

GPUResourceManager::GPUResourceManager(Device& device, CommandQueue& uploadQueue)
    : m_Device(device), m_UploadQueue(uploadQueue), m_Textures(device) {
    ThrowIfFailed(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CopyAllocator)));
    m_CopyCommandList = std::make_unique<CommandList>(m_Device, m_CopyAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CopyFence = std::make_unique<Fence>(m_Device);

    BeginCopyPass();
    m_Textures.Initialize(*m_CopyCommandList);
    EndAndSubmitCopyPass();
}

void GPUResourceManager::BeginCopyPass() {
    assert(!m_CopyPassActive && "Copy pass already active");
    ThrowIfFailed(m_CopyAllocator->Reset());
    m_CopyCommandList->Reset(m_CopyAllocator.Get());
    m_StagingBuffers.clear();
    m_CopyPassActive = true;
}

void GPUResourceManager::EndAndSubmitCopyPass() {
    assert(m_CopyPassActive && "Copy pass not active");
    m_CopyPassActive = false;

    ThrowIfFailed(m_CopyCommandList->GetHandle()->Close());
    ID3D12CommandList* const commandLists[]{m_CopyCommandList->GetHandle()};
    m_UploadQueue.ExecuteCommandLists(commandLists);

    const uint64_t fenceValue = m_CopyFence->Signal(m_UploadQueue.GetHandle());
    m_CopyFence->WaitForValue(fenceValue);

    m_StagingBuffers.clear();
    m_Textures.ReleasePendingUploads();
}

void GPUResourceManager::StageBuffer(Buffer& destination, const void* data, UINT64 size) {
    const BufferDesc stagingDesc{.Size = size, .HeapType = D3D12_HEAP_TYPE_UPLOAD};
    m_StagingBuffers.emplace_back(m_Device, m_UploadQueue, stagingDesc);
    Buffer& staging = m_StagingBuffers.back();
    staging.Write(data, size);

    auto* cmdList = m_CopyCommandList->GetHandle();

    const CD3DX12_RESOURCE_BARRIER toCopyDest{CD3DX12_RESOURCE_BARRIER::Transition(
        destination.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST)};
    cmdList->ResourceBarrier(1, &toCopyDest);

    cmdList->CopyBufferRegion(destination.Get(), 0, staging.Get(), 0, size);

    const CD3DX12_RESOURCE_BARRIER toGenericRead{CD3DX12_RESOURCE_BARRIER::Transition(
        destination.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ)};
    cmdList->ResourceBarrier(1, &toGenericRead);
}

std::unique_ptr<MeshBuffer> GPUResourceManager::StageMeshBuffer(const Mesh& mesh) {
    assert(m_CopyPassActive && "StageMeshBuffer must run inside a copy pass");
    auto meshBuffer = std::make_unique<MeshBuffer>(m_Device, m_UploadQueue, mesh);

    const UINT64 vertexBytes = mesh.Vertices.size() * sizeof(Vertex);
    if (vertexBytes != 0)
        StageBuffer(meshBuffer->GetVertexBuffer(), mesh.Vertices.data(), vertexBytes);

    const UINT64 indexBytes = mesh.Indices.size() * sizeof(uint32_t);
    if (indexBytes != 0)
        StageBuffer(meshBuffer->GetIndexBuffer(), mesh.Indices.data(), indexBytes);

    return meshBuffer;
}

std::unique_ptr<Texture> GPUResourceManager::StageTexture(const Image& image, bool isSRGB) {
    assert(m_CopyPassActive && "StageTexture must run inside a copy pass");
    return m_Textures.CreateTexture(image, isSRGB, *m_CopyCommandList);
}

GPUModelHandle GPUResourceManager::StageModelToVRAM(const Model& cpuModel) {
    assert(m_CopyPassActive && "StageModelToVRAM must run inside a copy pass");

    GPUModelHandle handle{m_NextModelId++};
    ModelRecord record;
    record.Submeshes.reserve(cpuModel.Meshes.size());

    std::unordered_map<int32_t, uint32_t> sourceToSrv;
    auto resolveTexture = [&](int32_t sourceIndex, uint32_t fallbackIndex) -> uint32_t {
        if (sourceIndex < 0)
            return fallbackIndex;
        if (auto it = sourceToSrv.find(sourceIndex); it != sourceToSrv.end())
            return it->second;

        uint32_t srvIndex = fallbackIndex;
        if (static_cast<size_t>(sourceIndex) < cpuModel.Textures.size()) {
            const TextureSource& source = cpuModel.Textures[sourceIndex];
            bool isSRGB{};
            std::unique_ptr<Image> image;
            if (const auto* file = std::get_if<TexturePath>(&source)) {
                isSRGB = file->IsSRGB;
                try {
                    image = std::make_unique<Image>(file->Path);
                } catch (const std::exception&) {
                    GE_CORE_WARN("GPUResourceManager: failed to load texture {}", file->Path.string());
                }
            } else if (const auto* embedded = std::get_if<TextureEmbedded>(&source)) {
                isSRGB = embedded->IsSRGB;
                try {
                    image = std::make_unique<Image>(embedded->Buffer.data(), embedded->Buffer.size());
                } catch (const std::exception&) {
                    GE_CORE_WARN("GPUResourceManager: failed to decode embedded texture");
                }
            }

            if (image) {
                auto texture = m_Textures.CreateTexture(*image, isSRGB, *m_CopyCommandList);
                srvIndex = texture->GetSrvIndex();
                record.OwnedTextures.push_back(std::move(texture));
            }
        }
        sourceToSrv[sourceIndex] = srvIndex;
        return srvIndex;
    };

    for (const Mesh& mesh : cpuModel.Meshes) {
        MaterialGPU material;
        material.AlbedoIndex = m_Textures.GetDefaultAlbedoIndex();
        material.NormalIndex = m_Textures.GetDefaultNormalIndex();
        material.RoughnessMetallicIndex = m_Textures.GetDefaultRoughnessMetallicIndex();

        if (mesh.MaterialIndex < cpuModel.Materials.size()) {
            const Material& cpuMaterial = cpuModel.Materials[mesh.MaterialIndex];
            material.AlbedoIndex = resolveTexture(cpuMaterial.BaseColorTextureIndex, material.AlbedoIndex);
            material.NormalIndex = resolveTexture(cpuMaterial.NormalTextureIndex, material.NormalIndex);
            material.RoughnessMetallicIndex =
                resolveTexture(cpuMaterial.RoughnessMetallic, material.RoughnessMetallicIndex);
        }

        record.Submeshes.push_back({StageMeshBuffer(mesh), material});
    }

    m_GPUModels[handle.Id] = std::move(record);
    return handle;
}

void GPUResourceManager::RegisterModelHandle(ModelHandle cpuHandle, GPUModelHandle gpuHandle) {
    m_ModelToGPU[cpuHandle.Id] = gpuHandle;
}

GPUModelHandle GPUResourceManager::GetGPUHandle(ModelHandle cpuHandle) const {
    const auto it = m_ModelToGPU.find(cpuHandle.Id);
    return it != m_ModelToGPU.end() ? it->second : GPUModelHandle{};
}

uint32_t GPUResourceManager::GetSubmeshCount(GPUModelHandle handle) const {
    const auto it = m_GPUModels.find(handle.Id);
    return it != m_GPUModels.end() ? static_cast<uint32_t>(it->second.Submeshes.size()) : 0u;
}

const GPUSubmesh& GPUResourceManager::GetSubmesh(GPUModelHandle handle, uint32_t submeshIndex) const {
    const auto it = m_GPUModels.find(handle.Id);
    assert(it != m_GPUModels.end() && "Unknown GPUModelHandle");
    assert(submeshIndex < it->second.Submeshes.size() && "Submesh index out of range");
    return it->second.Submeshes[submeshIndex];
}

void GPUResourceManager::Shutdown() {
    if (m_CopyPassActive)
        EndAndSubmitCopyPass();
    m_CopyFence->Flush(m_UploadQueue.GetHandle());

    m_Textures.ReleasePendingUploads();
    m_GPUModels.clear();
    m_ModelToGPU.clear();
    m_StagingBuffers.clear();
}

} // namespace GEngine
