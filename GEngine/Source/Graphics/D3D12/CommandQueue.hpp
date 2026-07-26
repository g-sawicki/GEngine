#pragma once

#include "Device.hpp"

namespace GEngine {

class CommandQueue {
  public:
    CommandQueue(Device& device, D3D12_COMMAND_LIST_TYPE type);

    [[nodiscard]] ID3D12CommandQueue* GetHandle() const noexcept { return m_CommandQueue.Get(); }

    void ExecuteCommandLists(std::span<ID3D12CommandList* const> commandLists) noexcept;

  private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
};

} // namespace GEngine
