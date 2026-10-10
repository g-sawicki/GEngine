#pragma once

#include "Core/Utility/Defines.hpp"
#include "Core/Utility/FreeListAllocator.hpp"

#include <cstdint>

namespace GEngine {

class Device;

class DescriptorHeap {
  public:
    DescriptorHeap() = default;
    DescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_DESC desc);

    GE_NO_COPY_DEFAULT_MOVE(DescriptorHeap)

    [[nodiscard]] uint32_t Allocate() { return m_Allocator.Allocate(); }
    void Deallocate(uint32_t index) { m_Allocator.Deallocate(index); }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const noexcept {
        return {m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr +
                static_cast<SIZE_T>(index) * m_DescriptorSize};
    }

    void Release() noexcept { m_DescriptorHeap.Reset(); }

    [[nodiscard]] ID3D12DescriptorHeap* Get() const noexcept { return m_DescriptorHeap.Get(); }

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC m_Desc{};
    UINT m_DescriptorSize{};

    FreeListAllocator m_Allocator;
};

} // namespace GEngine
