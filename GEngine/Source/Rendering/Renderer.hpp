#pragma once

#include "Scene/AssetManager.hpp"
#include "Scene/Scene.hpp"

#include "Core/Utility/Defines.hpp"
#include "Core/Utility/Image.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/DescriptorHeap.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/SwapChain.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"
#include "Rendering/GpuResourceCache.hpp"
#include "Rendering/RenderPass/ForwardLightingPass.hpp"
#include "Rendering/RenderPass/ShadowPass.hpp"
#include "Rendering/RenderPass/ToneMapPass.hpp"
#include "Rendering/SkyboxRenderer.hpp"
#include "Rendering/UploadEngine.hpp"

#include <array>
#include <memory>
#include <vector>

namespace GEngine {

class Renderer {
  public:
    Renderer(HWND hwnd, uint32_t width, uint32_t height, bool useWarp, uint32_t shadowMapSize);

    GE_NO_COPY_NO_MOVE(Renderer)

    void Destroy();

    /// Reports live DXGI objects. Debug builds only; call after the Renderer has been destroyed.
    static void ReportLiveObjects();

    void Render(const Scene& scene, const AssetManager& assetManager);
    void OnResize(uint32_t width, uint32_t height);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device.IsDeviceRemoved(); }

    void SetVSync(bool enabled) noexcept { m_SwapChain.SetVSync(enabled); }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_SwapChain.IsVSyncEnabled(); }

  private:
    struct FrameResource {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        std::unique_ptr<Buffer> SceneInfoConstantBuffer;
        std::unique_ptr<Buffer> CascadedShadowMapsDataConstantBuffer;
        std::unique_ptr<Buffer> LightDataStructuredBuffer;
        std::vector<std::unique_ptr<Buffer>> ObjectConstantBuffers;
        uint64_t FenceValue{};
    };
    using FrameResources = std::array<FrameResource, SwapChain::NumFrames>;

    std::unique_ptr<Buffer> CreateConstantBuffer(UINT64 size);

    FrameResources CreateFrameResources();
    void CreateRenderTargets(uint32_t width, uint32_t height);

    void UpdateGpuScene(const Scene& scene, const AssetManager& assetManager);

    void FrustumCulling(const Camera& camera, const CascadedShadowMapsData& cascadedShadowMapsData);

    Device m_Device;
    CommandQueue m_CommandQueue;
    Fence m_Fence;
    SwapChain m_SwapChain;
    UploadEngine m_UploadEngine;
    GpuResourceCache m_GpuResources;

    FrameResources m_FrameResources{};

    // Render pass resources
    Texture m_HdrTexture{};
    Texture m_PresentTarget{};
    Texture m_DepthTexture{};
    Texture m_ShadowMapTexture{};

    // Render passes
    RenderPass::ShadowPass m_ShadowPass;
    RenderPass::ForwardLightingPass m_ForwardLightingPass;
    RenderPass::ToneMapPass m_ToneMapPass;

    SkyboxRenderer m_SkyboxRenderer;

    std::vector<RenderItem> m_RenderItems;
};

} // namespace GEngine
