#include "PCH.hpp"

#include "Fence.hpp"

#include "Common.hpp"

namespace GEngine {

Fence::Fence(Device& device) {
    ThrowIfFailed(device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_FenceEvent && "Failed to create fence event.");
}

Fence::~Fence() {
    if (m_FenceEvent) {
        ::CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }
}

uint64_t Fence::Signal(ID3D12CommandQueue* commandQueue) {
    const uint64_t value{++m_NextFenceValue};
    ThrowIfFailed(commandQueue->Signal(m_Fence.Get(), value));
    return value;
}

void Fence::WaitForValue(uint64_t fenceValue, std::chrono::milliseconds duration) {
    if (m_Fence->GetCompletedValue() < fenceValue) {
        ThrowIfFailed(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
        const DWORD ms{(duration == std::chrono::milliseconds::max()) ? INFINITE
                                                                      : static_cast<DWORD>(duration.count())};
        ::WaitForSingleObject(m_FenceEvent, ms);
    }
}

void Fence::Flush(ID3D12CommandQueue* commandQueue) {
    const uint64_t value{Signal(commandQueue)};
    WaitForValue(value);
}

} // namespace GEngine
