#pragma once

#include "DescriptorHeap.hpp"
#include "Device.hpp"

#include <cstdint>
#include <dxgiformat.h>

namespace GEngine {

class CommandQueue;
class Image;

class Texture {
  public:
    Texture(Device& device, CommandQueue& commandQueue, DescriptorHandle descriptorHandle, const Image& image);
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
