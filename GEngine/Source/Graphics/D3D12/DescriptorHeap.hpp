#pragma once

namespace GEngine {

class Device;

struct DescriptorHandle {
    CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
    UINT Index;
};

struct DescriptorRange {
    D3D12_CPU_DESCRIPTOR_HANDLE Base{};
    UINT Stride{};
    UINT Count{};

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const noexcept {
        return {Base.ptr + static_cast<SIZE_T>(index) * Stride};
    }
};

class DescriptorHeap {
  public:
    DescriptorHeap() = default;
    DescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors,
                   D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    DescriptorHeap(const DescriptorHeap&) = delete;
    DescriptorHeap& operator=(const DescriptorHeap&) = delete;
    DescriptorHeap(DescriptorHeap&&) = default;
    DescriptorHeap& operator=(DescriptorHeap&&) = default;

    [[nodiscard]] UINT AllocateIndex();
    [[nodiscard]] DescriptorHandle Allocate();
    [[nodiscard]] DescriptorRange AllocateRange(UINT count);

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const noexcept {
        return {m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr +
                static_cast<SIZE_T>(index) * m_DescriptorSize};
    }

    void Reset() noexcept { m_CurrentIndex = 0; }
    void Release() noexcept { m_DescriptorHeap.Reset(); }

    [[nodiscard]] ID3D12DescriptorHeap* Get() const noexcept { return m_DescriptorHeap.Get(); }
    [[nodiscard]] UINT GetNumDescriptors() const noexcept { return m_NumDescriptors; }
    [[nodiscard]] UINT GetDescriptorSize() const noexcept { return m_DescriptorSize; }
    [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetType() const noexcept { return m_Type; }
    [[nodiscard]] bool IsShaderVisible() const noexcept {
        return (m_Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0;
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type{};
    D3D12_DESCRIPTOR_HEAP_FLAGS m_Flags{};
    UINT m_NumDescriptors{};
    UINT m_DescriptorSize{};
    UINT m_CurrentIndex{};
};

} // namespace GEngine
