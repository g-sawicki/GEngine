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
#include "Rendering/GPUResourceManager.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Rendering/RenderPass/ForwardLightingPass.hpp"
#include "Rendering/RenderPass/ShadowPass.hpp"
#include "Rendering/RenderPass/SkyboxPass.hpp"
#include "Rendering/RenderPass/ToneMapPass.hpp"

#include <memory>
#include <vector>

namespace GEngine {

class Renderer {
  public:
    struct FrameResource {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        std::unique_ptr<Buffer> SceneInfoConstantBuffer;
        std::unique_ptr<Buffer> CascadedShadowMapsDataConstantBuffer;
        std::unique_ptr<Buffer> LightDataStructuredBuffer;
        std::vector<std::unique_ptr<Buffer>> ObjectConstantBuffers;
        uint64_t FenceValue{};
    };

    Renderer() = default;

    GE_NO_COPY_NO_MOVE(Renderer)

    void Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp, uint32_t shadowMapSize);
    void Destroy();

    void Render(const Scene& scene, const AssetManager& assetManager);
    void OnResize(uint32_t width, uint32_t height);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device && m_Device->IsDeviceRemoved(); }

    void SetVSync(bool enabled) noexcept { m_VSync = enabled; }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_VSync; }
    [[nodiscard]] bool IsTearingSupported() const noexcept { return m_TearingSupported; }

  private:
    std::unique_ptr<Buffer> CreateConstantBuffer(UINT64 size);

    void CreateRenderTargets(uint32_t width, uint32_t height);

    void FlushPendingUploads(const Scene& scene, const AssetManager& assetManager);

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<CommandQueue> m_UploadQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    std::unique_ptr<GPUResourceManager> m_GPUResourceManager;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    // Render pass resources
    Texture m_HdrTexture{};
    Texture m_PresentTarget{};
    Texture m_DepthTexture{};
    Texture m_ShadowMapTexture{};

    // Render passes
    std::unique_ptr<RenderPass::ShadowPass> m_ShadowPass;
    std::unique_ptr<RenderPass::ForwardLightingPass> m_ForwardLighting;
    std::unique_ptr<RenderPass::SkyboxPass> m_SkyboxPass;
    std::unique_ptr<RenderPass::ToneMapPass> m_ToneMapPass;

    std::unique_ptr<MeshBuffer> m_SkyboxMeshBuffer;
    std::unique_ptr<Texture> m_SkyboxTexture;
    const Image* m_SkyboxSource{};

    std::vector<RenderItem> m_RenderItems;

    bool m_VSync{false};
    bool m_TearingSupported{};
};

} // namespace GEngine
