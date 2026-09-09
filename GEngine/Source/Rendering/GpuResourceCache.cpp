#include "PCH.hpp"

#include "Rendering/GpuResourceCache.hpp"

#include "Core/Utility/Image.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/UploadEngine.hpp"

namespace GEngine {

namespace {

[[nodiscard]] DXGI_FORMAT PickFormat(const DXGI_FORMAT format, const bool isSRGB) noexcept {
    if (isSRGB && format == DXGI_FORMAT_R8G8B8A8_UNORM)
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    return format;
}

} // namespace

GpuResourceCache::GpuResourceCache(Device& device, UploadEngine& uploads) : m_Device(device), m_UploadEngine(uploads) {}

void GpuResourceCache::EnsureDefaultTextures() {
    if (m_DefaultAlbedo)
        return;

    static constexpr uint8_t kWhitePixel[]{255, 255, 255, 255};
    static constexpr uint8_t kFlatNormalPixel[]{128, 128, 255, 255};
    static constexpr uint8_t kDefaultRoughnessMetallicPixel[]{255, 255, 0, 255};

    m_DefaultAlbedo = CreateTexture(Image::FromRGBA8(kWhitePixel, 1, 1), false);
    m_DefaultNormal = CreateTexture(Image::FromRGBA8(kFlatNormalPixel, 1, 1), false);
    m_DefaultRoughnessMetallic = CreateTexture(Image::FromRGBA8(kDefaultRoughnessMetallicPixel, 1, 1), false);
}

std::shared_ptr<Texture> GpuResourceCache::CreateTexture(const Image& image, const bool isSRGB) {
    auto texture = std::make_shared<Texture>();
    const TextureDesc desc{
        .Width = image.GetWidth(),
        .Height = image.GetHeight(),
        .Format = PickFormat(image.GetFormat(), isSRGB),
        .Usage = TextureUsage::ShaderResource,
    };
    texture->Create(m_Device, desc);
    const SubresourceData data{image.GetData().data()};
    m_UploadEngine.UploadTexture(*texture, {&data, 1});
    return texture;
}

std::shared_ptr<Texture> GpuResourceCache::CreateTextureFromSource(const TextureSource& source) {
    bool isSRGB{};
    std::string dedupKey;

    std::unique_ptr<Image> image;
    if (const auto* file = std::get_if<TexturePath>(&source)) {
        isSRGB = file->IsSRGB;
        dedupKey = file->Path.string() + (isSRGB ? "#srgb" : "#linear");
        if (auto it = m_Textures.find(dedupKey); it != m_Textures.end())
            return it->second;

        try {
            image = std::make_unique<Image>(file->Path);
        } catch (const std::exception&) {
            GE_CORE_WARN("GpuResourceCache: failed to load texture {}", file->Path.string());
        }
    } else if (const auto* embedded = std::get_if<TextureEmbedded>(&source)) {
        isSRGB = embedded->IsSRGB;
        try {
            image = std::make_unique<Image>(embedded->Buffer.data(), embedded->Buffer.size());
        } catch (const std::exception&) {
            GE_CORE_WARN("GpuResourceCache: failed to decode embedded texture");
        }
    }

    if (!image)
        return {};

    std::shared_ptr<Texture> texture = CreateTexture(*image, isSRGB);
    if (!dedupKey.empty())
        m_Textures[dedupKey] = texture;
    return texture;
}

uint32_t GpuResourceCache::ResolveTextureIndex(int32_t sourceIndex, uint32_t fallbackIndex,
                                               const std::vector<TextureSource>& sources,
                                               std::unordered_map<int32_t, uint32_t>& inModelDedup) {
    if (sourceIndex < 0)
        return fallbackIndex;
    if (auto it = inModelDedup.find(sourceIndex); it != inModelDedup.end())
        return it->second;

    uint32_t srvIndex = fallbackIndex;
    if (static_cast<size_t>(sourceIndex) < sources.size()) {
        if (std::shared_ptr<Texture> texture = CreateTextureFromSource(sources[sourceIndex]); texture)
            srvIndex = texture->GetSrvIndex();
    }
    inModelDedup[sourceIndex] = srvIndex;
    return srvIndex;
}

void GpuResourceCache::StageMeshGeometry(MeshGPU& outGeometry, const Mesh& mesh) {
    outGeometry.VertexStride = static_cast<UINT>(sizeof(Vertex));
    outGeometry.IndexCount = static_cast<UINT>(mesh.Indices.size());

    if (!mesh.Vertices.empty()) {
        const UINT64 vertexBytes = mesh.Vertices.size() * sizeof(Vertex);
        const BufferDesc vbDesc{.Size = vertexBytes, .HeapType = D3D12_HEAP_TYPE_DEFAULT};
        outGeometry.VertexBuffer = Buffer{m_Device, vbDesc};
        m_UploadEngine.UploadBuffer(outGeometry.VertexBuffer, mesh.Vertices.data(), vertexBytes);
    }
    if (!mesh.Indices.empty()) {
        const UINT64 indexBytes = mesh.Indices.size() * sizeof(uint32_t);
        const BufferDesc ibDesc{.Size = indexBytes, .HeapType = D3D12_HEAP_TYPE_DEFAULT};
        outGeometry.IndexBuffer = Buffer{m_Device, ibDesc};
        m_UploadEngine.UploadBuffer(outGeometry.IndexBuffer, mesh.Indices.data(), indexBytes);
    }
}

MeshGPU GpuResourceCache::StageMesh(const Mesh& mesh) {
    MeshGPU meshGpu;
    StageMeshGeometry(meshGpu, mesh);
    return meshGpu;
}

void GpuResourceCache::StageModel(const Model& model, const uint32_t cpuModelId) {
    EnsureDefaultTextures();

    GpuModel gpuModel;
    gpuModel.Meshes.reserve(model.Meshes.size());

    // Deduplicate texture uploads that appear more than once within a single model.
    std::unordered_map<int32_t, uint32_t> inModelDedup;

    for (const Mesh& mesh : model.Meshes) {
        GpuMesh gpuMesh;
        StageMeshGeometry(gpuMesh.Geometry, mesh);

        gpuMesh.Material.AlbedoIndex = m_DefaultAlbedo->GetSrvIndex();
        gpuMesh.Material.NormalIndex = m_DefaultNormal->GetSrvIndex();
        gpuMesh.Material.RoughnessMetallicIndex = m_DefaultRoughnessMetallic->GetSrvIndex();

        if (mesh.MaterialIndex < model.Materials.size()) {
            const Material& material = model.Materials[mesh.MaterialIndex];
            gpuMesh.Material.AlbedoIndex = ResolveTextureIndex(
                material.BaseColorTextureIndex, gpuMesh.Material.AlbedoIndex, model.Textures, inModelDedup);
            gpuMesh.Material.NormalIndex = ResolveTextureIndex(
                material.NormalTextureIndex, gpuMesh.Material.NormalIndex, model.Textures, inModelDedup);
            gpuMesh.Material.RoughnessMetallicIndex = ResolveTextureIndex(
                material.RoughnessMetallic, gpuMesh.Material.RoughnessMetallicIndex, model.Textures, inModelDedup);
        }

        gpuModel.Meshes.push_back(std::move(gpuMesh));
    }

    m_Models[cpuModelId] = std::move(gpuModel);
}

const GpuModel* GpuResourceCache::FindModel(const uint32_t cpuModelId) const noexcept {
    const auto it = m_Models.find(cpuModelId);
    return it != m_Models.end() ? &it->second : nullptr;
}

bool GpuResourceCache::HasModel(const uint32_t cpuModelId) const noexcept {
    return m_Models.contains(cpuModelId);
}

uint32_t GpuResourceCache::GetDefaultAlbedoIndex() const noexcept {
    return m_DefaultAlbedo ? m_DefaultAlbedo->GetSrvIndex() : INVALID_BINDLESS_INDEX;
}

uint32_t GpuResourceCache::GetDefaultNormalIndex() const noexcept {
    return m_DefaultNormal ? m_DefaultNormal->GetSrvIndex() : INVALID_BINDLESS_INDEX;
}

uint32_t GpuResourceCache::GetDefaultRoughnessMetallicIndex() const noexcept {
    return m_DefaultRoughnessMetallic ? m_DefaultRoughnessMetallic->GetSrvIndex() : INVALID_BINDLESS_INDEX;
}

} // namespace GEngine
