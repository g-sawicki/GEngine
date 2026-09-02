#pragma once

#include "Core/Utility/Defines.hpp"
#include "Core/Utility/Image.hpp"
#include "D3D12Common.hpp"
#include "DescriptorHeap.hpp"
#include "Device.hpp"

#include <cstdint>
#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

namespace GEngine {

class CommandQueue;
class CommandList;
class Image;

enum class TextureUsage : uint32_t {
    None = 0,
    RenderTarget = 1 << 0,
    DepthStencil = 1 << 1,
    ShaderResource = 1 << 2,
    UnorderedAccess = 1 << 3,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasUsage(TextureUsage mask, TextureUsage flag) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

struct TextureDesc {
    uint32_t Width{1};
    uint32_t Height{1};
    uint16_t Depth{1};
    uint16_t MipCount{1};
    DXGI_FORMAT Format{DXGI_FORMAT_UNKNOWN};
    TextureUsage Usage{TextureUsage::None};
    D3D12_CLEAR_VALUE ClearValue{};
};

struct TextureFormatInfo {
    DXGI_FORMAT Resource{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT ShaderResource{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT RenderTarget{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT DepthStencil{DXGI_FORMAT_UNKNOWN};
};

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

class Texture {
  public:
    Texture() = default;

    GE_NO_COPY(Texture)
    GE_DEFAULT_MOVE(Texture)

    void Create(Device& device, const TextureDesc& desc);
    void CreateFromResource(ID3D12Resource* resource, const TextureDesc& desc, D3D12_RESOURCE_STATES initialState);
    void CreateFromImage(Device& device, CommandQueue& commandQueue, const TextureDesc& desc, const Image& image);
    void CreateFromRGBA8(Device& device, CommandQueue& commandQueue, uint32_t width, uint32_t height,
                         const void* rgba8);
    void Reset() noexcept;
    void Transition(CommandList& commandList, D3D12_RESOURCE_STATES state);

    [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] const TextureDesc& GetDesc() const noexcept { return m_Desc; }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const noexcept { return m_RtvHandle; }
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(uint16_t arraySlice = 0) const noexcept {
        return m_DsvRange.GetCpuHandle(arraySlice);
    }
    [[nodiscard]] uint32_t GetSrvIndex() const noexcept { return m_SrvIndex; }
    [[nodiscard]] uint32_t GetUavIndex() const noexcept { return m_UavIndex; }

  private:
    void UploadPixels(Device& device, CommandQueue& commandQueue, const TextureDesc& desc, uint32_t sourceRowPitch,
                      const void* pixels);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    TextureDesc m_Desc{};
    D3D12_RESOURCE_STATES m_State{D3D12_RESOURCE_STATE_COMMON};

    D3D12_CPU_DESCRIPTOR_HANDLE m_RtvHandle{INVALID_HANDLE};
    DescriptorRange m_DsvRange;
    uint32_t m_SrvIndex{INVALID_BINDLESS_INDEX};
    uint32_t m_UavIndex{INVALID_BINDLESS_INDEX};
};

} // namespace GEngine
