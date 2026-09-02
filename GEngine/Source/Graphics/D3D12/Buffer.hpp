#pragma once

#include "Core/Utility/Defines.hpp"
#include "D3D12Common.hpp"
#include "Device.hpp"

#include <memory>

namespace GEngine {

class CommandQueue;

enum class BufferMiscFlags : uint32_t {
    None = 0,
    ConstantBuffer = 1 << 0, // requires 256-byte alignment for CBV
};

[[nodiscard]] constexpr bool HasFlag(BufferMiscFlags flags, BufferMiscFlags flag) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

struct BufferDesc {
    UINT64 Size{};
    D3D12_HEAP_TYPE HeapType{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_FLAGS Flags{D3D12_RESOURCE_FLAG_NONE};
    BufferMiscFlags MiscFlags{BufferMiscFlags::None};
};

class Buffer {
  public:
    Buffer() = default;
    Buffer(Device& device, CommandQueue& commandQueue, const BufferDesc& desc, const void* initialData = nullptr);

    GE_NO_COPY_DEFAULT_MOVE(Buffer)

    [[nodiscard]] ID3D12Resource* Get() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept {
        return m_Resource->GetGPUVirtualAddress();
    }
    [[nodiscard]] UINT64 GetSize() const noexcept { return m_Size; }

    [[nodiscard]] uint32_t GetSrvIndex() const noexcept { return m_SrvIndex; }
    void CreateStructuredBufferSRV(Device& device, UINT numElements, UINT strideInBytes);

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVBV(UINT stride) const noexcept {
        return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<UINT>(m_Size),
                .StrideInBytes = stride};
    }
    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIBV(DXGI_FORMAT format = DXGI_FORMAT_R32_UINT) const noexcept {
        return {.BufferLocation = m_Resource->GetGPUVirtualAddress(),
                .SizeInBytes = static_cast<UINT>(m_Size),
                .Format = format};
    }

    void Write(const void* data, UINT64 size);

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    UINT64 m_Size{};
    D3D12_HEAP_TYPE m_HeapType{};
    uint32_t m_SrvIndex{INVALID_BINDLESS_INDEX};
};

} // namespace GEngine
