#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"

namespace GEngine::RenderPass {

class DeferredLightingPass {
  public:
    DeferredLightingPass(Device& device);

    GE_NO_COPY_NO_MOVE(DeferredLightingPass)

    void Dispatch(CommandList& commandList, uint32_t albedoIndex, uint32_t normalIndex, uint32_t roughnessMetallicIndex,
                  uint32_t depthIndex, uint32_t shadowMapIndex, uint32_t hdrIndex, const Buffer& sceneInfoCB,
                  const Buffer& cascadedShadowMapsDataCB, uint32_t width, uint32_t height);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
