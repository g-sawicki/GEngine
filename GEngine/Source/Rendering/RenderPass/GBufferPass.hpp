#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"

#include <span>

namespace GEngine::RenderPass {

struct GBufferFormats {
    DXGI_FORMAT AlbedoFormat{DXGI_FORMAT_R8G8B8A8_UNORM};
    DXGI_FORMAT NormalFormat{DXGI_FORMAT_R10G10B10A2_UNORM};
    DXGI_FORMAT RoughnessMetallicFormat{DXGI_FORMAT_R8G8_UNORM};
};

struct GBufferTextures {
    Texture AlbedoTexture;
    Texture NormalTexture;
    Texture RoughnessMetallicTexture;
};

class GBufferPass {
  public:
    GBufferPass(Device& device, const GBufferFormats& gBufferFormats, const DXGI_FORMAT depthFormat);

    GE_NO_COPY_NO_MOVE(GBufferPass)

    void OnRender(CommandList& commandList, const GBufferTextures& gBufferTextures, const Texture& depthTexture,
                  const Buffer& sceneInfoCB, std::span<const RenderItem> renderItems);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                             const GBufferFormats& gBufferFormats, const DXGI_FORMAT depthFormat);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
