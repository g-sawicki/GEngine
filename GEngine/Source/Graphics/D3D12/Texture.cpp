#include "PCH.hpp"

#include "Texture.hpp"

#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Core/Utility/Image.hpp"
#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"
#include "Fence.hpp"

namespace GEngine {

namespace {

[[nodiscard]] D3D12_RESOURCE_STATES GetInitialState(const TextureDesc& desc) noexcept {
    if (HasUsage(desc.Usage, TextureUsage::DepthStencil))
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (HasUsage(desc.Usage, TextureUsage::UnorderedAccess))
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (HasUsage(desc.Usage, TextureUsage::ShaderResource))
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget))
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    return D3D12_RESOURCE_STATE_COMMON;
}

} // namespace

void Texture::Create(Device& device, const TextureDesc& desc) {
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

    const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
    const D3D12_RESOURCE_STATES initialState = GetInitialState(desc);
    m_State = initialState;

    // Clear
    const D3D12_CLEAR_VALUE* pClearValue{};
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget) || HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        pClearValue = &desc.ClearValue;
    }

    const TextureFormatInfo formatInfo = GetTextureFormatInfo(desc.Format);
    const CD3DX12_RESOURCE_DESC resourceDesc{
        CD3DX12_RESOURCE_DESC::Tex2D(formatInfo.Resource, static_cast<UINT64>(desc.Width),
                                     static_cast<UINT>(desc.Height), desc.Depth, desc.MipCount, 1, 0, flags)};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                        pClearValue, IID_PPV_ARGS(&m_Resource)));

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

void Texture::CreateFromResource(ID3D12Resource* resource, const TextureDesc& desc,
                                 D3D12_RESOURCE_STATES initialState) {
    m_Resource = resource;
    m_Desc = desc;
    m_State = initialState;
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

void Texture::CreateFromImage(Device& device, CommandQueue& commandQueue, const TextureDesc& desc, const Image& image) {
    m_Desc = desc;

    const CD3DX12_RESOURCE_DESC resourceDesc{
        CD3DX12_RESOURCE_DESC::Tex2D(desc.Format, image.GetWidth(), image.GetHeight())};
    const CD3DX12_HEAP_PROPERTIES defaultHeap{D3D12_HEAP_TYPE_DEFAULT};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                        IID_PPV_ARGS(&m_Resource)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 uploadSize{};
    device.Get()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, nullptr, nullptr, &uploadSize);

    const CD3DX12_HEAP_PROPERTIES uploadHeap{D3D12_HEAP_TYPE_UPLOAD};
    const CD3DX12_RESOURCE_DESC uploadDesc{CD3DX12_RESOURCE_DESC::Buffer(uploadSize)};
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    ThrowIfFailed(device.Get()->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&uploadBuffer)));

    void* mappedData{};
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, &mappedData));
    auto* destination = static_cast<uint8_t*>(mappedData) + footprint.Offset;
    const auto& source = image.GetData();
    const uint32_t sourceRowPitch = static_cast<uint32_t>(image.GetRowPitch());
    const uint32_t destinationRowPitch = footprint.Footprint.RowPitch;
    for (uint32_t row{}; row < image.GetHeight(); ++row) {
        std::memcpy(destination + row * destinationRowPitch, source.data() + row * sourceRowPitch, sourceRowPitch);
    }
    uploadBuffer->Unmap(0, nullptr);

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    ThrowIfFailed(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
    CommandList commandList{device, allocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT};
    commandList.Reset(allocator.Get());

    D3D12_TEXTURE_COPY_LOCATION destinationLocation{
        .pResource = m_Resource.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };
    D3D12_TEXTURE_COPY_LOCATION sourceLocation{
        .pResource = uploadBuffer.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = footprint,
    };
    commandList.GetHandle()->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
        m_Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)};
    commandList.GetHandle()->ResourceBarrier(1, &barrier);
    commandList.Close();

    ID3D12CommandList* commandLists[]{commandList.GetHandle()};
    commandQueue.ExecuteCommandLists(commandLists);
    Fence fence{device};
    fence.Flush(commandQueue.GetHandle());
    m_State = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    if (m_SrvIndex == INVALID_BINDLESS_INDEX)
        m_SrvIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = desc.Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = 1},
    };
    device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc,
                                           device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_SrvIndex));
}

void Texture::Transition(CommandList& commandList, D3D12_RESOURCE_STATES state) {
    if (m_State == state)
        return;

    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(m_Resource.Get(), m_State, state)};
    commandList.GetHandle()->ResourceBarrier(1, &barrier);
    m_State = state;
}

} // namespace GEngine
