#pragma once

#include "Device.hpp"

namespace GEngine {

class Fence {

  public:
    explicit Fence(Device& device);
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&&) = delete;
    Fence& operator=(Fence&&) = delete;

    [[nodiscard]] ID3D12Fence* GetHandle() const noexcept { return m_Fence.Get(); }

    /// Signal the fence from @p commandQueue and return the new fence value.
    [[nodiscard]] uint64_t Signal(ID3D12CommandQueue* commandQueue);

    /// Busy-wait until the fence reaches @p fenceValue.
    void WaitForValue(uint64_t fenceValue, std::chrono::milliseconds duration = std::chrono::milliseconds::max());

    /// Signal the fence and immediately wait for it (drain the queue).
    void Flush(ID3D12CommandQueue* commandQueue);

  private:
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    HANDLE m_FenceEvent{};
    uint64_t m_NextFenceValue{};
};

} // namespace GEngine
