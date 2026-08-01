#pragma once

namespace GEngine {

class Device {
  public:
    explicit Device(IDXGIAdapter4* adapter);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    [[nodiscard]] ID3D12Device8* Get() const noexcept { return m_Device.Get(); }

    [[nodiscard]] bool IsDeviceRemoved() const noexcept;

    /// Must be called before any D3D12 objects are created.
    static void EnableDebugLayer();
    static Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory();
    static Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory6* dxgiFactory, bool useWarp);

  private:
    Microsoft::WRL::ComPtr<ID3D12Device8> m_Device;
};

} // namespace GEngine
