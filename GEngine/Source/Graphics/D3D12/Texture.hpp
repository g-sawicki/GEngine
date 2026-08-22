#pragma once

#include "Core/Utility/Image.hpp"
#include "DescriptorHeap.hpp"
#include "Device.hpp"

#include <cstdint>
#include <d3d12.h>
#include <dxgiformat.h>
#include <optional>

namespace GEngine {

class CommandQueue;
class Image;

struct TextureDesc {
    uint32_t Width{};
    uint32_t Height{};
    DXGI_FORMAT Format{};
};

class Texture {
  public:
    Texture(Device& device, CommandQueue& commandQueue, DescriptorHandle srvHandle, const TextureDesc& desc,
            const Image& image);

    Texture(Device& device, DescriptorHandle rtvHandle, const TextureDesc& desc,
            const std::optional<D3D12_CLEAR_VALUE>& clearValue = std::nullopt);
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    [[nodiscard]] ID3D12Resource* Get() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept { return m_DescriptorHandle.cpuHandle; }
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const noexcept { return m_DescriptorHandle.gpuHandle; }
    [[nodiscard]] DXGI_FORMAT GetFormat() const noexcept { return m_Format; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    DXGI_FORMAT m_Format{DXGI_FORMAT_UNKNOWN};
    DescriptorHandle m_DescriptorHandle{};
};

} // namespace GEngine
