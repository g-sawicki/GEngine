#pragma once

#include "Core/Utility/Defines.hpp"
#include "DescriptorHeap.hpp"

namespace GEngine {

class CommandList;

class Device {
  public:
    explicit Device(bool useWarp);

    GE_NO_COPY_NO_MOVE(Device)

    [[nodiscard]] ID3D12Device8* Get() const noexcept { return m_Device.Get(); }
    [[nodiscard]] IDXGIFactory6* GetFactory() const noexcept { return m_DxgiFactory.Get(); }
    [[nodiscard]] DescriptorHeap& GetRtvDescriptorHeap() noexcept { return m_RtvDescriptorHeap; }
    [[nodiscard]] DescriptorHeap& GetDsvDescriptorHeap() noexcept { return m_DsvDescriptorHeap; }
    [[nodiscard]] DescriptorHeap& GetShaderResourceDescriptorHeap() noexcept { return m_ShaderResourceDescriptorHeap; }

    [[nodiscard]] bool IsDeviceRemoved() const noexcept;

    static void EnableDebugLayer();

    void SetDescriptorHeaps(CommandList& commandList);

  private:
    static Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory();
    static Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory6* dxgiFactory, bool useWarp);

    Microsoft::WRL::ComPtr<IDXGIFactory6> m_DxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device8> m_Device;

    DescriptorHeap m_RtvDescriptorHeap{};
    DescriptorHeap m_DsvDescriptorHeap{};
    DescriptorHeap m_ShaderResourceDescriptorHeap{};
};

} // namespace GEngine
