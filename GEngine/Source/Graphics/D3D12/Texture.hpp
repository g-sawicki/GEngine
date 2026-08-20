#pragma once

#include "Core/Utility/Image.hpp"
#include "DescriptorHeap.hpp"
#include "Device.hpp"

#include <cstdint>
#include <dxgiformat.h>

namespace GEngine {

class CommandQueue;
class Image;

struct TextureDesc {
    uint32_t Width;
    uint32_t Height;
    DXGI_FORMAT Format;
};

class Texture {
  public:
    Texture(Device& device, CommandQueue& commandQueue, DescriptorHandle descriptorHandle, const TextureDesc& desc,
            const Image& image);
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    [[nodiscard]] ID3D12Resource* Get() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const noexcept { return m_SRV; }
    [[nodiscard]] DXGI_FORMAT GetFormat() const noexcept { return m_Format; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    DXGI_FORMAT m_Format{DXGI_FORMAT_UNKNOWN};
    D3D12_GPU_DESCRIPTOR_HANDLE m_SRV{};
};

} // namespace GEngine
