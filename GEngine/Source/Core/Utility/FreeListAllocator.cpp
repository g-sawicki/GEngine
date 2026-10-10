#include "PCH.hpp"

#include "FreeListAllocator.hpp"

#include <numeric>

namespace GEngine {

FreeListAllocator::FreeListAllocator(uint32_t capacity) : m_FreeList(capacity), m_Allocated(capacity, false) {
    std::iota(m_FreeList.begin(), m_FreeList.end(), 0);
}

uint32_t FreeListAllocator::Allocate() {
    if (m_FreeList.empty())
        throw std::out_of_range("FreeListAllocator exhausted.");

    const uint32_t index = m_FreeList.back();
    m_FreeList.pop_back();
    m_Allocated[index] = true;
    return index;
}

void FreeListAllocator::Deallocate(uint32_t index) {
    if (index >= m_Allocated.size())
        throw std::out_of_range("FreeListAllocator index is out of range.");
    if (!m_Allocated[index])
        throw std::logic_error("FreeListAllocator index is not allocated.");

    m_FreeList.push_back(index);
    m_Allocated[index] = false;
}

} // namespace GEngine
