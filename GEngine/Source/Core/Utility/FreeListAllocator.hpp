#pragma once

#include <cstdint>
#include <vector>

namespace GEngine {

class FreeListAllocator {
  public:
    FreeListAllocator() = default;
    explicit FreeListAllocator(uint32_t capacity);

    uint32_t Allocate();
    void Deallocate(uint32_t index);

  private:
    std::vector<uint32_t> m_FreeList{};
    std::vector<bool> m_Allocated{};
};

} // namespace GEngine
