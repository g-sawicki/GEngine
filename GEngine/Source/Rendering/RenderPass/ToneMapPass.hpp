#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"

namespace GEngine::RenderPass {

class ToneMapPass {
  public:
    explicit ToneMapPass(Device& device);

    ToneMapPass(const ToneMapPass&) = delete;
    ToneMapPass& operator=(const ToneMapPass&) = delete;
    ToneMapPass(ToneMapPass&&) = delete;
    ToneMapPass& operator=(ToneMapPass&&) = delete;

    void Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex, Buffer& sceneInfoBuffer,
                  uint32_t width, uint32_t height);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;
};

} // namespace GEngine::RenderPass
