#pragma once

#include "Device.hpp"

namespace GEngine {

class PipelineState {
  public:
    PipelineState(Device& device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
    PipelineState(Device& device, const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

    [[nodiscard]] ID3D12PipelineState* Get() const noexcept { return m_PipelineState.Get(); }

  private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};

} // namespace GEngine
