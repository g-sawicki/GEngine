#include "PCH.hpp"

#include "Texture.hpp"

#include "CommandList.hpp"
#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"

namespace GEngine {

namespace {

[[nodiscard]] D3D12_RESOURCE_STATES GetInitialState(const TextureDesc& desc) noexcept {
    if (HasUsage(desc.Usage, TextureUsage::DepthStencil))
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (HasUsage(desc.Usage, TextureUsage::UnorderedAccess))
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (HasUsage(desc.Usage, TextureUsage::ShaderResource))
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget))
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    return D3D12_RESOURCE_STATE_COMMON;
}

[[nodiscard]] uint32_t GetBytesPerPixel(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return 4;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return sizeof(float) * 4;
    default:
        throw std::runtime_error(
            std::format("Unsupported source format for image upload: {}", static_cast<int>(format)));
    }
}

} // namespace

Texture::Texture(ID3D12Resource* resource, const TextureDesc& desc, D3D12_RESOURCE_STATES initialState)
    : m_Resource(resource), m_Desc(desc), m_State(initialState) {}

void Texture::Create(Device& device, const TextureDesc& desc, std::span<const SubresourceData> initialData,
                     CommandList* copyCommandList, Microsoft::WRL::ComPtr<ID3D12Resource>* outStaging) {
    assert(initialData.empty() || (copyCommandList != nullptr && outStaging != nullptr) &&
                                      "Staged upload needs an open copy list and a staging sink");
    assert(initialData.empty() || initialData.size() == static_cast<size_t>(desc.Depth) * desc.MipCount &&
                                      "One SubresourceData entry per subresource required");

    m_Desc = desc;

    // Flags
    D3D12_RESOURCE_FLAGS flags{D3D12_RESOURCE_FLAG_NONE};
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (!HasUsage(desc.Usage, TextureUsage::ShaderResource))
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
    if (HasUsage(desc.Usage, TextureUsage::UnorderedAccess))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    const bool uploadsData = !initialData.empty();
    const D3D12_RESOURCE_STATES restingState = GetInitialState(desc);
    m_State = uploadsData ? D3D12_RESOURCE_STATE_COPY_DEST : restingState;

    // Clear
    const D3D12_CLEAR_VALUE* pClearValue{};
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget) || HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        pClearValue = &desc.ClearValue;
    }

    const TextureFormatInfo formatInfo = GetTextureFormatInfo(desc.Format);
    const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_RESOURCE_DESC resourceDesc{
        CD3DX12_RESOURCE_DESC::Tex2D(formatInfo.Resource, static_cast<UINT64>(desc.Width),
                                     static_cast<UINT>(desc.Height), desc.Depth, desc.MipCount, 1, 0, flags)};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, m_State,
                                                        pClearValue, IID_PPV_ARGS(&m_Resource)));

    if (uploadsData) {
        const UINT subresourceCount = static_cast<UINT>(desc.Depth) * desc.MipCount;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(subresourceCount);
        UINT64 totalBytes{};
        device.Get()->GetCopyableFootprints(&resourceDesc, 0, subresourceCount, 0, footprints.data(), nullptr, nullptr,
                                            &totalBytes);

        Microsoft::WRL::ComPtr<ID3D12Resource> staging;
        const CD3DX12_HEAP_PROPERTIES uploadHeap{D3D12_HEAP_TYPE_UPLOAD};
        const CD3DX12_RESOURCE_DESC uploadDesc{CD3DX12_RESOURCE_DESC::Buffer(totalBytes)};
        ThrowIfFailed(device.Get()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                            IID_PPV_ARGS(&staging)));

        void* mappedData{};
        ThrowIfFailed(staging->Map(0, nullptr, &mappedData));
        auto* stagingBytes = static_cast<uint8_t*>(mappedData);
        const uint32_t bytesPerPixel = GetBytesPerPixel(desc.Format);
        for (UINT subresource{}; subresource < subresourceCount; ++subresource) {
            const uint32_t mip = subresource % desc.MipCount;
            const uint32_t width = std::max(1u, desc.Width >> mip);
            const SubresourceData& source = initialData[subresource];
            const uint32_t sourceRowPitch =
                source.RowPitch != 0 ? static_cast<uint32_t>(source.RowPitch) : width * bytesPerPixel;

            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[subresource];
            auto* destination = stagingBytes + footprint.Offset;
            const auto* sourcePixels = static_cast<const uint8_t*>(source.Data);
            for (UINT row{}; row < footprint.Footprint.Height; ++row) {
                std::memcpy(destination + row * footprint.Footprint.RowPitch, sourcePixels + row * sourceRowPitch,
                            sourceRowPitch);
            }
        }
        staging->Unmap(0, nullptr);

        auto* cmdList = copyCommandList->GetHandle();
        for (UINT subresource{}; subresource < subresourceCount; ++subresource) {
            const D3D12_TEXTURE_COPY_LOCATION destinationLocation{
                .pResource = m_Resource.Get(),
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = subresource,
            };
            const D3D12_TEXTURE_COPY_LOCATION sourceLocation{
                .pResource = staging.Get(),
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = footprints[subresource],
            };
            cmdList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
        }

        const CD3DX12_RESOURCE_BARRIER barrier{
            CD3DX12_RESOURCE_BARRIER::Transition(m_Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, restingState)};
        cmdList->ResourceBarrier(1, &barrier);
        m_State = restingState;

        *outStaging = std::move(staging);
    }

    if (HasUsage(desc.Usage, TextureUsage::ShaderResource)) {
        if (m_SrvIndex == INVALID_BINDLESS_INDEX)
            m_SrvIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = formatInfo.ShaderResource,
            .ViewDimension = desc.Depth > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {.MipLevels = desc.MipCount},
        };
        if (desc.Depth > 1)
            srvDesc.Texture2DArray = {.MipLevels = desc.MipCount, .ArraySize = desc.Depth};

        const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_SrvIndex);
        device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, srvHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::RenderTarget)) {
        if (m_RtvHandle.ptr == INVALID_HANDLE)
            m_RtvHandle = device.GetRtvDescriptorHeap().Allocate().CpuHandle;

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
            .Format = formatInfo.RenderTarget,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };

        device.Get()->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_RtvHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::UnorderedAccess)) {
        if (m_UavIndex == INVALID_BINDLESS_INDEX)
            m_UavIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
            .Format = formatInfo.RenderTarget,
            .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
            .Texture2D = {.MipSlice = 0},
        };
        const D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_UavIndex);
        device.Get()->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, uavHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        if (m_DsvRange.Base.ptr == 0)
            m_DsvRange = device.GetDsvDescriptorHeap().AllocateRange(desc.Depth);
        for (uint16_t arraySlice{}; arraySlice < desc.Depth; ++arraySlice) {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
                .Format = formatInfo.DepthStencil,
                .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            };
            if (desc.Depth > 1) {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray = {.MipSlice = 0, .FirstArraySlice = arraySlice, .ArraySize = 1};
            }
            device.Get()->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, m_DsvRange.GetCpuHandle(arraySlice));
        }
    }
}

void Texture::Reset() noexcept {
    m_Resource.Reset();
    m_Desc = {};
    m_State = D3D12_RESOURCE_STATE_COMMON;
    m_RtvHandle = {INVALID_HANDLE};
    m_DsvRange = {};
    m_SrvIndex = INVALID_BINDLESS_INDEX;
    m_UavIndex = INVALID_BINDLESS_INDEX;
}

void Texture::Transition(CommandList& commandList, D3D12_RESOURCE_STATES state) {
    if (m_State == state)
        return;

    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(m_Resource.Get(), m_State, state)};
    commandList.GetHandle()->ResourceBarrier(1, &barrier);
    m_State = state;
}

} // namespace GEngine
