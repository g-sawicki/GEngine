#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"

namespace GEngine::RenderPass {

class BloomPass {
  public:
    explicit BloomPass(Device& device);

    GE_NO_COPY_NO_MOVE(BloomPass)

    void Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex, bool horizontal,
                  Buffer& sceneInfoBuffer, uint32_t width, uint32_t height);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
