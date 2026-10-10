#include "PCH.hpp"

#include "Texture.hpp"

#include "CommandList.hpp"
#include "Core/Utility/Math.hpp"
#include "D3D12Common.hpp"

#include <utility>

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

Texture::Texture(Texture&& other) noexcept
    : m_Resource(std::move(other.m_Resource)), m_Desc(std::exchange(other.m_Desc, {})),
      m_RtvIndices(std::move(other.m_RtvIndices)),
      m_DsvIndices(std::move(other.m_DsvIndices)),
      m_SrvIndex(std::exchange(other.m_SrvIndex, INVALID_BINDLESS_INDEX)),
      m_UavIndex(std::exchange(other.m_UavIndex, INVALID_BINDLESS_INDEX)),
      m_Device(std::exchange(other.m_Device, nullptr)) {
    other.m_RtvIndices.clear();
    other.m_DsvIndices.clear();
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Reset();
        m_Resource = std::move(other.m_Resource);
        m_Desc = std::exchange(other.m_Desc, {});
        m_RtvIndices = std::move(other.m_RtvIndices);
        m_DsvIndices = std::move(other.m_DsvIndices);
        m_SrvIndex = std::exchange(other.m_SrvIndex, INVALID_BINDLESS_INDEX);
        m_UavIndex = std::exchange(other.m_UavIndex, INVALID_BINDLESS_INDEX);
        m_Device = std::exchange(other.m_Device, nullptr);
        other.m_RtvIndices.clear();
        other.m_DsvIndices.clear();
    }
    return *this;
}

Texture::~Texture() {
    Reset();
}

Texture::Texture(Device& device, const TextureDesc& desc) {
    Create(device, desc);
}

void Texture::Create(Device& device, const TextureDesc& desc) {
    Reset();
    assert(!desc.IsCubeMap || desc.DepthOrArraySize == 6);

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
        DXGI_FORMAT format =
            HasUsage(desc.Usage, TextureUsage::DepthStencil) ? formatInfo.DepthStencil : formatInfo.RenderTarget;
        if (clearValue.Format == DXGI_FORMAT_UNKNOWN) {
            clearValue.Format = format;
        } else {
            assert(clearValue.Format == format);
        }
        pClearValue = &clearValue;
    }

    const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_RESOURCE_DESC resourceDesc{CD3DX12_RESOURCE_DESC::Tex2D(
        formatInfo.Resource, static_cast<UINT64>(desc.Width), static_cast<UINT>(desc.Height), desc.DepthOrArraySize,
        desc.MipCount, 1, 0, flags)};
    ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                        desc.InitialState, pClearValue, IID_PPV_ARGS(&m_Resource)));

    m_Desc = desc;
    m_Device = &device;

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

        auto& srvDescriptorHeap = device.GetShaderResourceDescriptorHeap();
        m_SrvIndex = srvDescriptorHeap.Allocate();
        const D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvDescriptorHeap.GetCpuHandle(m_SrvIndex);
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

        auto& uavDescriptorHeap = device.GetShaderResourceDescriptorHeap();
        m_UavIndex = uavDescriptorHeap.Allocate();
        const D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = uavDescriptorHeap.GetCpuHandle(m_UavIndex);
        device.Get()->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, uavHandle);
    }

    if (HasUsage(desc.Usage, TextureUsage::RenderTarget)) {
        auto& rtvDescriptorHeap = device.GetRtvDescriptorHeap();
        m_RtvIndices.resize(desc.DepthOrArraySize, INVALID_BINDLESS_INDEX);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{
            .Format = formatInfo.RenderTarget,
            .ViewDimension =
                (desc.DepthOrArraySize > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DARRAY : D3D12_RTV_DIMENSION_TEXTURE2D,
        };

        for (uint32_t slice{}; slice < desc.DepthOrArraySize; ++slice) {
            if (desc.DepthOrArraySize > 1)
                rtvDesc.Texture2DArray = {.MipSlice = 0, .FirstArraySlice = slice, .ArraySize = 1, .PlaneSlice = 0};
            else
                rtvDesc.Texture2D = {.MipSlice = 0, .PlaneSlice = 0};
            const uint32_t index = rtvDescriptorHeap.Allocate();
            m_RtvIndices[slice] = index;
            const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCpuHandle(index);
            device.Get()->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, rtvHandle);
        }
    }

    if (HasUsage(desc.Usage, TextureUsage::DepthStencil)) {
        auto& dsvDescriptorHeap = device.GetDsvDescriptorHeap();
        m_DsvIndices.resize(desc.DepthOrArraySize, INVALID_BINDLESS_INDEX);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
            .Format = formatInfo.DepthStencil,
            .ViewDimension =
                (desc.DepthOrArraySize > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DARRAY : D3D12_DSV_DIMENSION_TEXTURE2D,
        };

        for (uint16_t slice{}; slice < desc.DepthOrArraySize; ++slice) {
            if (desc.DepthOrArraySize > 1)
                dsvDesc.Texture2DArray = {.MipSlice = 0, .FirstArraySlice = slice, .ArraySize = 1};
            else
                dsvDesc.Texture2D = {.MipSlice = 0};
            const uint32_t index = dsvDescriptorHeap.Allocate();
            m_DsvIndices[slice] = index;
            const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap.GetCpuHandle(index);
            device.Get()->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, dsvHandle);
        }
    }
}

void Texture::Reset() noexcept {
    ReleaseDescriptors();
    m_Resource.Reset();
    m_Desc = {};
    m_RtvIndices.clear();
    m_DsvIndices.clear();
}

void Texture::ReleaseDescriptors() noexcept {
    if (m_Device != nullptr) {
        auto& rtvDescriptorHeap = m_Device->GetRtvDescriptorHeap();
        for (const uint32_t index : m_RtvIndices)
            if (index != INVALID_BINDLESS_INDEX)
                rtvDescriptorHeap.Deallocate(index);

        auto& dsvDescriptorHeap = m_Device->GetDsvDescriptorHeap();
        for (const uint32_t index : m_DsvIndices)
            if (index != INVALID_BINDLESS_INDEX)
                dsvDescriptorHeap.Deallocate(index);

        auto& shaderResourceDescriptorHeap = m_Device->GetShaderResourceDescriptorHeap();
        if (m_SrvIndex != INVALID_BINDLESS_INDEX)
            shaderResourceDescriptorHeap.Deallocate(m_SrvIndex);
        if (m_UavIndex != INVALID_BINDLESS_INDEX)
            shaderResourceDescriptorHeap.Deallocate(m_UavIndex);
    }
    m_RtvIndices.clear();
    m_DsvIndices.clear();
    m_SrvIndex = INVALID_BINDLESS_INDEX;
    m_UavIndex = INVALID_BINDLESS_INDEX;
    m_Device = nullptr;
}

} // namespace GEngine
