#pragma once

#include "Core/Utility/Defines.hpp"
#include "D3D12Common.hpp"
#include "Device.hpp"

#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <cstdint>
#include <span>

namespace GEngine {

enum class TextureUsage : uint32_t {
    None = 0,
    RenderTarget = 1 << 0,
    DepthStencil = 1 << 1,
    ShaderResource = 1 << 2,
    UnorderedAccess = 1 << 3,
};

inline constexpr TextureUsage operator|(const TextureUsage a, const TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr bool HasUsage(const TextureUsage mask, const TextureUsage flag) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

struct TextureDesc {
    uint32_t Width{1};
    uint32_t Height{1};
    uint16_t DepthOrArraySize{1};
    uint16_t MipCount{1};
    DXGI_FORMAT Format{DXGI_FORMAT_UNKNOWN};
    TextureUsage Usage{TextureUsage::None};
    bool IsCubeMap{false};
    D3D12_CLEAR_VALUE ClearValue{};
    D3D12_RESOURCE_STATES InitialState{D3D12_RESOURCE_STATE_COMMON};
};

struct SubresourceData {
    const void* Data{};
    UINT64 RowPitch{};
    UINT64 SlicePitch{};
};

struct TextureFormatInfo {
    DXGI_FORMAT Resource{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT ShaderResource{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT RenderTarget{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT DepthStencil{DXGI_FORMAT_UNKNOWN};
};

class Texture {
  public:
    Texture() = default;
    Texture(ID3D12Resource* resource, const TextureDesc& desc);
    Texture(Device& device, const TextureDesc& desc);

    GE_NO_COPY(Texture)
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

    void Create(Device& device, const TextureDesc& desc);
    void Reset() noexcept;

    [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] const TextureDesc& GetDesc() const noexcept { return m_Desc; }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(uint16_t arraySlice = 0) const {
        assert(m_Device != nullptr);
        return m_Device->GetRtvDescriptorHeap().GetCpuHandle(m_RtvIndices.at(arraySlice));
    }
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(uint16_t arraySlice = 0) const {
        assert(m_Device != nullptr);
        return m_Device->GetDsvDescriptorHeap().GetCpuHandle(m_DsvIndices.at(arraySlice));
    }
    [[nodiscard]] uint32_t GetSrvIndex() const noexcept { return m_SrvIndex; }
    [[nodiscard]] uint32_t GetUavIndex() const noexcept { return m_UavIndex; }

  private:
    void ReleaseDescriptors() noexcept;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    TextureDesc m_Desc{};

    std::vector<uint32_t> m_RtvIndices;
    std::vector<uint32_t> m_DsvIndices;
    uint32_t m_SrvIndex{INVALID_BINDLESS_INDEX};
    uint32_t m_UavIndex{INVALID_BINDLESS_INDEX};
    Device* m_Device{};
};

} // namespace GEngine
