#include "PCH.hpp"

#include "LinearAllocator.hpp"

#include "Core/Utility/Math.hpp"

namespace GEngine {

LinearAllocator::LinearAllocator(Device& device, size_t size) : m_Size(size) {
    const BufferDesc desc{.Size = size, .HeapType = D3D12_HEAP_TYPE_UPLOAD};
    m_Buffer = std::make_unique<Buffer>(device, desc);
    m_Head = m_Buffer->Map();
    m_GpuHead = m_Buffer->GetGPUVirtualAddress();
}

LinearAllocator::~LinearAllocator() {
    m_Buffer->Unmap();
}

void LinearAllocator::Reset() {
    m_Index = 0;
}

std::pair<void*, D3D12_GPU_VIRTUAL_ADDRESS> LinearAllocator::Allocate(size_t size, size_t alignment) {
    assert(alignment != 0);
    m_Index = RoundUp(m_Index, alignment);
    void* ptr = static_cast<uint8_t*>(m_Head) + m_Index;
    D3D12_GPU_VIRTUAL_ADDRESS gpuPtr = m_GpuHead + m_Index;
    assert(gpuPtr % alignment == 0);
    m_Index += size;
    assert(m_Index <= m_Size);
    return {ptr, gpuPtr};
}

} // namespace GEngine
