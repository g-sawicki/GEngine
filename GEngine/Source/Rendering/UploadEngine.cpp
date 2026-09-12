#include "PCH.hpp"

#include "Rendering/UploadEngine.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"

#include <cstring>

namespace GEngine {

namespace {

constexpr UINT64 kStagingAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
constexpr UINT64 kStagingChunkSize = 1u << 20;

[[nodiscard]] D3D12_RESOURCE_STATES GetRestingState(const TextureUsage usage) noexcept {
    if (HasUsage(usage, TextureUsage::DepthStencil))
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (HasUsage(usage, TextureUsage::UnorderedAccess))
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (HasUsage(usage, TextureUsage::ShaderResource))
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if (HasUsage(usage, TextureUsage::RenderTarget))
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    return D3D12_RESOURCE_STATE_COMMON;
}

} // namespace

UploadEngine::UploadEngine(Device& device)
    : m_Device(device), m_Queue(CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT)), m_Fence(Fence(device)) {
    ThrowIfFailed(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocator)));
    m_CommandList = std::make_unique<CommandList>(device, m_Allocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

UploadEngine::~UploadEngine() = default;

void UploadEngine::Begin() {
    assert(!m_BatchActive);
    ThrowIfFailed(m_Allocator->Reset());
    m_CommandList->Reset(m_Allocator.Get());
    m_StagingChunks.clear();
    m_BatchActive = true;
}

void UploadEngine::Submit(const bool wait) {
    assert(m_BatchActive);
    m_BatchActive = false;

    ThrowIfFailed(m_CommandList->GetHandle()->Close());
    ID3D12CommandList* const commandLists[]{m_CommandList->GetHandle()};
    m_Queue.ExecuteCommandLists(commandLists);

    const uint64_t fenceValue = m_Fence.Signal(m_Queue.GetHandle());
    if (wait)
        m_Fence.WaitForValue(fenceValue);

    m_StagingChunks.clear();
}

UploadEngine::StagingAllocation UploadEngine::AllocateStaging(const UINT64 size) {
    assert(m_BatchActive);

    StagingChunk* chunk{};
    for (auto& candidate : m_StagingChunks) {
        const UINT64 aligned = RoundUp<kStagingAlignment>(candidate.Used);
        if (aligned + size <= candidate.Buffer->GetDesc().Size) {
            chunk = &candidate;
            break;
        }
    }
    if (chunk == nullptr) {
        const UINT64 chunkSize = std::max(kStagingChunkSize, RoundUp<kStagingAlignment>(size) + kStagingAlignment);
        const BufferDesc stagingDesc{.Size = chunkSize, .HeapType = D3D12_HEAP_TYPE_UPLOAD};
        m_StagingChunks.emplace_back(StagingChunk{std::make_unique<Buffer>(m_Device, stagingDesc), 0});
        chunk = &m_StagingChunks.back();
    }

    const UINT64 offset = RoundUp<kStagingAlignment>(chunk->Used);
    chunk->Used = offset + size;

    auto* const base = static_cast<std::byte*>(chunk->Buffer->Map());
    return {base + offset, chunk->Buffer->GetResource(), offset};
}

void UploadEngine::UploadBuffer(Buffer& dst, const void* data, const UINT64 size) {
    if (size == 0)
        return;

    const StagingAllocation staging = AllocateStaging(size);
    std::memcpy(staging.Cpu, data, size);

    auto* const cmdList = m_CommandList->GetHandle();
    const CD3DX12_RESOURCE_BARRIER toCopyDest{CD3DX12_RESOURCE_BARRIER::Transition(
        dst.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)};
    cmdList->ResourceBarrier(1, &toCopyDest);

    cmdList->CopyBufferRegion(dst.GetResource(), 0, staging.Resource, staging.Offset, size);

    const CD3DX12_RESOURCE_BARRIER toGenericRead{CD3DX12_RESOURCE_BARRIER::Transition(
        dst.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ)};
    cmdList->ResourceBarrier(1, &toGenericRead);
}

void UploadEngine::UploadTexture(Texture& dst, const std::span<const SubresourceData> subresources) {
    const TextureDesc& desc = dst.GetDesc();
    const UINT subresourceCount = static_cast<UINT>(desc.DepthOrArraySize) * desc.MipCount;
    assert(subresources.size() == subresourceCount);

    const CD3DX12_RESOURCE_DESC resourceDesc{
        CD3DX12_RESOURCE_DESC::Tex2D(desc.Format, static_cast<UINT64>(desc.Width), desc.Height, desc.DepthOrArraySize,
                                     desc.MipCount, 1, 0, D3D12_RESOURCE_FLAG_NONE)};

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(subresourceCount);
    std::vector<UINT> rowCounts(subresourceCount);
    std::vector<UINT64> tightRowSizes(subresourceCount);
    UINT64 totalBytes{};
    m_Device.Get()->GetCopyableFootprints(&resourceDesc, 0, subresourceCount, 0, footprints.data(), rowCounts.data(),
                                          tightRowSizes.data(), &totalBytes);

    const StagingAllocation staging = AllocateStaging(totalBytes);
    auto* stagingBase = staging.Cpu;

    for (UINT i{}; i < subresourceCount; ++i) {
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[i];
        const auto* src = static_cast<const std::byte*>(subresources[i].Data);
        const UINT64 sourceStride = subresources[i].RowPitch != 0 ? subresources[i].RowPitch : tightRowSizes[i];
        auto* dstPixels = stagingBase + footprint.Offset;
        for (UINT row{}; row < rowCounts[i]; ++row) {
            std::memcpy(dstPixels + static_cast<UINT64>(row) * footprint.Footprint.RowPitch, src, tightRowSizes[i]);
            src += sourceStride;
        }
    }

    auto* const cmdList = m_CommandList->GetHandle();
    const D3D12_RESOURCE_STATES restingState = GetRestingState(desc.Usage);

    const CD3DX12_RESOURCE_BARRIER toCopyDest{CD3DX12_RESOURCE_BARRIER::Transition(
        dst.GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)};
    cmdList->ResourceBarrier(1, &toCopyDest);

    for (UINT i{}; i < subresourceCount; ++i) {
        D3D12_TEXTURE_COPY_LOCATION source{};
        source.pResource = staging.Resource;
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint = {staging.Offset + footprints[i].Offset, footprints[i].Footprint};

        D3D12_TEXTURE_COPY_LOCATION destination{};
        destination.pResource = dst.GetResource();
        destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        destination.SubresourceIndex = i;

        cmdList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    }

    const CD3DX12_RESOURCE_BARRIER toResting{
        CD3DX12_RESOURCE_BARRIER::Transition(dst.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, restingState)};
    cmdList->ResourceBarrier(1, &toResting);
}

} // namespace GEngine
