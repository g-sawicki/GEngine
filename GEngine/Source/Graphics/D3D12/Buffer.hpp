#pragma once

#include "Device.hpp"

namespace GEngine {

class Buffer {
  public:
    Buffer(Device& device, UINT64 size, const void* initialData = nullptr);
    Buffer(Device& device, const D3D12_HEAP_PROPERTIES& heapProps, const D3D12_RESOURCE_DESC& desc,
           const void* initialData = nullptr);

    [[nodiscard]] ID3D12Resource* Get() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept {
        return m_Resource->GetGPUVirtualAddress();
    }
    [[nodiscard]] UINT64 GetSize() const noexcept { return m_Size; }

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVBV(UINT stride) const noexcept {
        return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<UINT>(m_Size),
                .StrideInBytes = stride};
    }
    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIBV(DXGI_FORMAT format = DXGI_FORMAT_R16_UINT) const noexcept {
        return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<UINT>(m_Size),
                .Format = format};
    }

    void Write(const void* data, UINT64 size);

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    UINT64 m_Size{};
};

} // namespace GEngine
