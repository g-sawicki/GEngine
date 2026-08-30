#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"

namespace GEngine::RenderPass {

class SkyboxPass {
  public:
    SkyboxPass(Device& device, const Texture& colorTexture, const Texture& depthTexture);

    SkyboxPass(const SkyboxPass&) = delete;
    SkyboxPass& operator=(const SkyboxPass&) = delete;
    SkyboxPass(SkyboxPass&&) = delete;
    SkyboxPass& operator=(SkyboxPass&&) = delete;

    void OnRender(CommandList& commandList, const MeshBuffer& cubeMesh, const Texture& colorTexture,
                  const Texture& depthTexture, uint32_t skyboxSrvIndex, Buffer& sceneInfoCB);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;
};

} // namespace GEngine::RenderPass
