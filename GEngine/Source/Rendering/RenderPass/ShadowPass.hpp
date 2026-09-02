#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"

namespace GEngine::RenderPass {

class ShadowPass {
  public:
    ShadowPass(Device& device, DXGI_FORMAT depthStencilFormat);

    GE_NO_COPY_NO_MOVE(ShadowPass)

    void OnRender(CommandList& commandList, Texture& shadowMapTexture, Buffer& lightDataConstantBuffer,
                  uint32_t cascadeCount, std::span<const RenderItem> renderItems);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;

    DXGI_FORMAT m_DepthStencilFormat{DXGI_FORMAT_UNKNOWN};
};

} // namespace GEngine::RenderPass
