#pragma once

#include "Scene/Scene.hpp"

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
#include "Rendering/MeshBuffer.hpp"
#include "Rendering/RenderItem.hpp"
#include "Rendering/RenderPass/ForwardLightingPass.hpp"
#include "Rendering/RenderPass/ShadowPass.hpp"
#include "Rendering/RenderPass/SkyboxPass.hpp"
#include "Rendering/RenderPass/ToneMapPass.hpp"

#include <memory>
#include <unordered_map>
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

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp, uint32_t shadowMapSize);
    void Destroy();

    void Render(const Scene& scene);
    void OnResize(uint32_t width, uint32_t height);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device && m_Device->IsDeviceRemoved(); }

    void SetVSync(bool enabled) noexcept { m_VSync = enabled; }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_VSync; }
    [[nodiscard]] bool IsTearingSupported() const noexcept { return m_TearingSupported; }

    std::unique_ptr<Texture> CreateTexture(const Image& image);

  private:
    struct ModelResources {
        std::vector<std::unique_ptr<MeshBuffer>> Meshes;
        std::vector<MaterialGPU> Materials;
        std::vector<std::unique_ptr<Texture>> Textures;
    };

    std::unique_ptr<MeshBuffer> CreateMeshBuffer(const Mesh& mesh);
    std::unique_ptr<Buffer> CreateConstantBuffer(UINT64 size);

    void CreateRenderTargets(uint32_t width, uint32_t height);

    void EnsureDefaultMaterial();
    const ModelResources& EnsureModelResources(const Model& model);

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    Texture m_HdrTexture{};
    Texture m_PresentTarget{};
    Texture m_DepthTexture{};
    Texture m_ShadowMapTexture{};

    std::unique_ptr<RenderPass::ShadowPass> m_ShadowPass;
    std::unique_ptr<RenderPass::ForwardLightingPass> m_ForwardLighting;
    std::unique_ptr<RenderPass::SkyboxPass> m_SkyboxPass;
    std::unique_ptr<RenderPass::ToneMapPass> m_ToneMapPass;

    std::unique_ptr<MeshBuffer> m_SkyboxMeshBuffer;

    std::vector<RenderItem> m_RenderItems;
    std::unordered_map<const Model*, ModelResources> m_ModelCache;

    Texture m_DefaultAlbedo;
    Texture m_DefaultNormal;
    Texture m_DefaultRoughnessMetallic;
    MaterialGPU m_DefaultMaterial{};

    bool m_VSync{false};
    bool m_TearingSupported{};
};

} // namespace GEngine
