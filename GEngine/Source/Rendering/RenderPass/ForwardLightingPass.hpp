#pragma once

#include "Core/World.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Rendering/RenderItem.hpp"

namespace GEngine::RenderPass {

class ForwardLightingPass {
  public:
    ForwardLightingPass(Device& device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat);
    ~ForwardLightingPass();

    ForwardLightingPass(const ForwardLightingPass&) = delete;
    ForwardLightingPass& operator=(const ForwardLightingPass&) = delete;
    ForwardLightingPass(ForwardLightingPass&&) = delete;
    ForwardLightingPass& operator=(ForwardLightingPass&&) = delete;

    void OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
                  Buffer& sceneInfoConstantBuffer, Buffer& lightDataConstantBuffer,
                  D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRV, std::span<const RenderItem> renderItems);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;

    DXGI_FORMAT m_RenderTargetFormat{DXGI_FORMAT_UNKNOWN};
    DXGI_FORMAT m_DepthStencilFormat{DXGI_FORMAT_UNKNOWN};
};

} // namespace GEngine::RenderPass
