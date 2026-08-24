#include "PCH.hpp"

#include "CommandList.hpp"
#include "Device.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

CommandList::CommandList(Device& device, ID3D12CommandAllocator* initialAllocator, D3D12_COMMAND_LIST_TYPE type) {
    ThrowIfFailed(device.Get()->CreateCommandList(0, type, initialAllocator, nullptr, IID_PPV_ARGS(&m_CommandList)));
    ThrowIfFailed(m_CommandList->Close());
}

void CommandList::Reset(ID3D12CommandAllocator* allocator) {
    ThrowIfFailed(allocator->Reset());
    ThrowIfFailed(m_CommandList->Reset(allocator, nullptr));
}

void CommandList::Close() {
    ThrowIfFailed(m_CommandList->Close());
}

} // namespace GEngine
