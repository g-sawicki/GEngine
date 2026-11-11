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

class ShadowPass {
  public:
    ShadowPass(Device& device, DXGI_FORMAT depthFormat);

    GE_NO_COPY_NO_MOVE(ShadowPass)

    void OnRender(CommandList& commandList, Texture& shadowMapTexture, Buffer& lightDataConstantBuffer,
                  std::span<const RenderItem> renderItems, const CascadeVisibility& cascadeVisibleItems);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                             DXGI_FORMAT depthFormat);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
