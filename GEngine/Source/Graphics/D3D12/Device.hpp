#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>

class Device {
public:
    explicit Device(ComPtr<IDXGIAdapter4> adapter);
    ~Device() = default;

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

    // Access the underlying D3D12 device.
    ID3D12Device2* Get() const { return m_Device.Get(); }

    // Must be called before any D3D12 objects are created.
    static void EnableDebugLayer();

private:
    ComPtr<ID3D12Device2> m_Device;
};
