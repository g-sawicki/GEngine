#include "PCH.hpp"

#include "CommandQueue.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

CommandQueue::CommandQueue(Device& device, D3D12_COMMAND_LIST_TYPE type) {
    D3D12_COMMAND_QUEUE_DESC desc{
        .Type = type,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0,
    };

    ThrowIfFailed(device.Get()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
}

void CommandQueue::ExecuteCommandLists(std::span<ID3D12CommandList* const> commandLists) noexcept {
    m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());
}

} // namespace GEngine
