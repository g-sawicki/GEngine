#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/Device.hpp"

#include <memory>
#include <utility>

namespace GEngine {

class LinearAllocator {
  public:
    LinearAllocator(Device& device, size_t size);
    ~LinearAllocator();

    GE_NO_COPY_NO_MOVE(LinearAllocator)

    void Reset();

    [[nodiscard]] std::pair<void*, D3D12_GPU_VIRTUAL_ADDRESS> Allocate(size_t size, size_t alignment);

  private:
    size_t m_Size{};
    size_t m_Index{};
    void* m_Head{};
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuHead{};
    std::unique_ptr<Buffer> m_Buffer;
};

} // namespace GEngine
