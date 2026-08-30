#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"

namespace GEngine::RenderPass {

class ForwardLightingPass {
  public:
    ForwardLightingPass(Device& device, const Texture& colorTexture, const Texture& depthTexture);

    ForwardLightingPass(const ForwardLightingPass&) = delete;
    ForwardLightingPass& operator=(const ForwardLightingPass&) = delete;
    ForwardLightingPass(ForwardLightingPass&&) = delete;
    ForwardLightingPass& operator=(ForwardLightingPass&&) = delete;

    void OnRender(CommandList& commandList, const Texture& colorTexture, const Texture& depthTexture,
                  const Texture& shadowMapTexture, const Buffer& sceneInfoCB, const Buffer& cascadedShadowMapsDataCB,
                  std::span<const RenderItem> renderItems);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;
};

} // namespace GEngine::RenderPass
