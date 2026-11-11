#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"
#include "Rendering/Culling.hpp"

#include <span>

namespace GEngine::RenderPass {

class ForwardLightingPass {
  public:
    ForwardLightingPass(Device& device, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat);

    GE_NO_COPY_NO_MOVE(ForwardLightingPass)

    void OnRender(CommandList& commandList, const Texture& colorTexture, const Texture& depthTexture,
                  const Texture& shadowMapTexture, const Buffer& sceneInfoCB, const Buffer& cascadedShadowMapsDataCB,
                  std::span<const RenderItem> renderItems, const VisibleItems& visibleItems);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                             DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
