#pragma once

#include "Core/Utility/Defines.hpp"

namespace GEngine {

class Device;

class CommandList {
  public:
    CommandList(Device& device, ID3D12CommandAllocator* initialAllocator, D3D12_COMMAND_LIST_TYPE type);

    GE_NO_COPY(CommandList)
    GE_DEFAULT_MOVE(CommandList)

    [[nodiscard]] ID3D12GraphicsCommandList4* GetHandle() const noexcept { return m_CommandList.Get(); }

    void Reset(ID3D12CommandAllocator* allocator);
    void Close();

  private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_CommandList;
};

} // namespace GEngine
