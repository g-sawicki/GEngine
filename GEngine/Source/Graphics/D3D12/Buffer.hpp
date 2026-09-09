#pragma once

#include "Core/Utility/Defines.hpp"
#include "D3D12Common.hpp"
#include "Device.hpp"

namespace GEngine {

enum class BufferMiscFlags : uint32_t {
    None = 0,
    ConstantBuffer = 1 << 0, // requires 256-byte alignment for CBV
};

[[nodiscard]] constexpr bool HasFlag(const BufferMiscFlags flags, const BufferMiscFlags flag) noexcept {
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
    Buffer(Device& device, const BufferDesc& desc);

    GE_NO_COPY_DEFAULT_MOVE(Buffer)

    [[nodiscard]] void* Map(UINT subresource = 0, const D3D12_RANGE* readRange = nullptr) const;
    void Unmap(UINT subresource = 0, const D3D12_RANGE* writtenRange = nullptr) const;

    void CreateStructuredBufferSRV(Device& device, UINT numElements, UINT strideInBytes);
    void CreateStructuredBufferUAV(Device& device, UINT numElements, UINT strideInBytes);

    [[nodiscard]] ID3D12Resource* GetResource() const noexcept { return m_Resource.Get(); }
    [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept {
        return m_Resource->GetGPUVirtualAddress();
    }
    [[nodiscard]] const BufferDesc& GetDesc() const noexcept { return m_BufferDesc; }

    [[nodiscard]] uint32_t GetSrvIndex() const noexcept { return m_SrvIndex; }
    [[nodiscard]] uint32_t GetUavIndex() const noexcept { return m_UavIndex; }

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVBV(UINT stride) const noexcept;
    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIBV(DXGI_FORMAT format = DXGI_FORMAT_R32_UINT) const noexcept;

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    BufferDesc m_BufferDesc{};

    uint32_t m_SrvIndex{INVALID_BINDLESS_INDEX};
    uint32_t m_UavIndex{INVALID_BINDLESS_INDEX};
};

} // namespace GEngine
