#include "PCH.hpp"

#include "Texture.hpp"

#include "CommandList.hpp"
#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"

namespace GEngine {

namespace {

[[nodiscard]] constexpr TextureFormatInfo GetTextureFormatInfo(DXGI_FORMAT format) noexcept {
    switch (format) {
    case DXGI_FORMAT_D16_UNORM:
        return {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D16_UNORM};
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_UNKNOWN,
                DXGI_FORMAT_D24_UNORM_S8_UINT};
    case DXGI_FORMAT_D32_FLOAT:
        return {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D32_FLOAT};
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_UNKNOWN,
                DXGI_FORMAT_D32_FLOAT_S8X24_UINT};
    default:
        return {format, format, format, DXGI_FORMAT_UNKNOWN};
    }
}

} // namespace

Texture::Texture(ID3D12Resource* resource, const TextureDesc& desc) : m_Resource(resource), m_Desc(desc) {}

void Texture::Create(Device& device, const TextureDesc& desc) {
    assert(!desc.IsCubeMap || desc.DepthOrArraySize == 6);
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

    const TextureFormatInfo formatInfo = GetTextureFormatInfo(desc.Format);
    D3D12_CLEAR_VALUE clearValue = desc.ClearValue;
    const D3D12_CLEAR_VALUE* pClearValue{};
    if (HasUsage(desc.Usage, TextureUsage::RenderTarget) || HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        if (clearValue.Format == DXGI_FORMAT_UNKNOWN) {
            clearValue.Format =
                HasUsage(desc.Usage, TextureUsage::DepthStencil) ? formatInfo.DepthStencil : formatInfo.RenderTarget;
        } else {
            if (HasUsage(desc.Usage, TextureUsage::DepthStencil))
                assert(clearValue.Format == formatInfo.DepthStencil);
            else
                assert(clearValue.Format == formatInfo.RenderTarget);
        }
        pClearValue = &clearValue;
    }

    const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_RESOURCE_DESC resourceDesc{CD3DX12_RESOURCE_DESC::Tex2D(
        formatInfo.Resource, static_cast<UINT64>(desc.Width), static_cast<UINT>(desc.Height), desc.DepthOrArraySize,
        desc.MipCount, 1, 0, flags)};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                        desc.InitialState, pClearValue, IID_PPV_ARGS(&m_Resource)));

    if (HasUsage(desc.Usage, TextureUsage::ShaderResource)) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = formatInfo.ShaderResource,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        };
        if (desc.IsCubeMap) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube = {.MostDetailedMip = 0, .MipLevels = desc.MipCount, .ResourceMinLODClamp = 0.0f};
        } else if (desc.DepthOrArraySize > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray = {.MostDetailedMip = 0,
                                      .MipLevels = desc.MipCount,
                                      .FirstArraySlice = 0,
                                      .ArraySize = desc.DepthOrArraySize,
                                      .PlaneSlice = 0,
                                      .ResourceMinLODClamp = 0.0f};
        } else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D = {
                .MostDetailedMip = 0, .MipLevels = desc.MipCount, .PlaneSlice = 0, .ResourceMinLODClamp = 0.0f};
        }

        m_SrvIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;
        const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_SrvIndex);
        device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, srvHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::UnorderedAccess)) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{.Format = formatInfo.ShaderResource};

        if (desc.DepthOrArraySize > 1) {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray = {
                .MipSlice = 0, .FirstArraySlice = 0, .ArraySize = desc.DepthOrArraySize, .PlaneSlice = 0};
        } else {
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D = {.MipSlice = 0, .PlaneSlice = 0};
        }

        m_UavIndex = device.GetShaderResourceDescriptorHeap().Allocate().Index;
        const D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = device.GetShaderResourceDescriptorHeap().GetCpuHandle(m_UavIndex);
        device.Get()->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, uavHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::RenderTarget)) {
        m_RtvRange = device.GetRtvDescriptorHeap().AllocateRange(desc.DepthOrArraySize);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{.Format = formatInfo.RenderTarget};
        if (desc.DepthOrArraySize > 1) {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            for (uint32_t slice{}; slice < desc.DepthOrArraySize; ++slice) {
                rtvDesc.Texture2DArray = {.MipSlice = 0, .FirstArraySlice = slice, .ArraySize = 1, .PlaneSlice = 0};
                device.Get()->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_RtvRange.GetCpuHandle(slice));
            }
        } else {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D = {.MipSlice = 0, .PlaneSlice = 0};
            device.Get()->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_RtvRange.GetCpuHandle(0));
        }
    }

    if (HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        m_DsvRange = device.GetDsvDescriptorHeap().AllocateRange(desc.DepthOrArraySize);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
            .Format = formatInfo.DepthStencil,
        };
        if (desc.DepthOrArraySize > 1) {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            for (uint16_t slice{}; slice < desc.DepthOrArraySize; ++slice) {
                dsvDesc.Texture2DArray = {.MipSlice = 0, .FirstArraySlice = slice, .ArraySize = 1};
                device.Get()->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, m_DsvRange.GetCpuHandle(slice));
            }
        } else {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D = {.MipSlice = 0};
            device.Get()->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, m_DsvRange.GetCpuHandle(0));
        }
    }
}

void Texture::Reset() noexcept {
    m_Resource.Reset();
    m_Desc = {};
    m_RtvRange = {};
    m_DsvRange = {};
    m_SrvIndex = INVALID_BINDLESS_INDEX;
    m_UavIndex = INVALID_BINDLESS_INDEX;
}

} // namespace GEngine
