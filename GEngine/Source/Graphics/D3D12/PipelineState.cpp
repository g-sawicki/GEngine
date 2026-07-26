#include "PCH.hpp"

#include "PipelineState.hpp"

#include "Common.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

PipelineState::PipelineState(Device& device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
    ThrowIfFailed(device.Get()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_PipelineState)));
}

} // namespace GEngine
