#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Rendering/RenderItem.hpp"

namespace GEngine::RenderPass {

class ShadowPass {
  public:
    ShadowPass(Device& device, DXGI_FORMAT depthStencilFormat);
    ~ShadowPass();

    ShadowPass(const ShadowPass&) = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;
    ShadowPass(ShadowPass&&) = delete;
    ShadowPass& operator=(ShadowPass&&) = delete;

    void OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, Buffer& lightDataConstantBuffer,
                  uint32_t cascadeIndex, std::span<const RenderItem> renderItems);

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;

    DXGI_FORMAT m_DepthStencilFormat{DXGI_FORMAT_UNKNOWN};
};

} // namespace GEngine::RenderPass
