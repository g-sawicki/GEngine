#pragma once

#include "Core/World.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Rendering/RenderItem.hpp"

namespace GEngine::RenderPass {

class ForwardLighting {
  public:
    ForwardLighting(Device& device, DXGI_FORMAT renderTargetFormat);
    ~ForwardLighting();

    ForwardLighting(const ForwardLighting&) = delete;
    ForwardLighting& operator=(const ForwardLighting&) = delete;
    ForwardLighting(ForwardLighting&&) = delete;
    ForwardLighting& operator=(ForwardLighting&&) = delete;

    void OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, const SceneInfo& sceneInfo,
                  Buffer& sceneInfoConstantBuffer, std::span<const RenderItem> renderItems);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;

    DXGI_FORMAT m_RenderTargetFormat{DXGI_FORMAT_UNKNOWN};
};

} // namespace GEngine::RenderPass
