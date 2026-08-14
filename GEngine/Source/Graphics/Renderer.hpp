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
        std::unique_ptr<Buffer> LightDataConstantBuffer;
        uint64_t FenceValue{};
    };

    Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp);
    void Destroy();

    void Render(const Scene& scene);
    void OnResize(uint32_t width, uint32_t height);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device && m_Device->IsDeviceRemoved(); }

    void SetVSync(bool enabled) noexcept { m_VSync = enabled; }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_VSync; }
    [[nodiscard]] bool IsTearingSupported() const noexcept { return m_TearingSupported; }

  private:
    struct ModelResources {
        std::vector<std::unique_ptr<MeshBuffer>> Meshes;
        std::vector<MaterialGPU> Materials;
        std::vector<std::unique_ptr<Texture>> Textures;
    };

    void UpdateRenderTargetViews();
    void CreateShadowMapSRV();
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthBuffer(uint32_t width, uint32_t height);

    std::unique_ptr<MeshBuffer> CreateMeshBuffer(const Mesh& mesh);
    std::unique_ptr<Buffer> CreateConstantBuffer(UINT64 size);
    std::unique_ptr<Texture> CreateTexture(const Image& image);

    void EnsureDefaultMaterial();
    const ModelResources& EnsureModelResources(const Model& model);

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    DescriptorHeap m_RTVDescriptorHeap{};
    DescriptorHeap m_DSVDescriptorHeap{};
    DescriptorHeap m_CbvSrvUavDescriptorHeap{};

    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandles[SwapChain::NumFrames]{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DepthStencilView{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ShadowMapView{};

    static constexpr DXGI_FORMAT s_DepthStencilResourceFormat{DXGI_FORMAT_R32_TYPELESS};
    static constexpr DXGI_FORMAT s_DepthStencilFormat{DXGI_FORMAT_D32_FLOAT};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer;

    static constexpr uint32_t s_ShadowMapSize{1024};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ShadowMapBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE m_ShadowMapSRV{};
    D3D12_RESOURCE_STATES m_ShadowMapState{D3D12_RESOURCE_STATE_DEPTH_WRITE};

    std::unique_ptr<RenderPass::ForwardLightingPass> m_ForwardLighting;
    std::unique_ptr<RenderPass::ShadowPass> m_ShadowPass;

    std::vector<RenderItem> m_RenderItems;
    std::vector<std::unique_ptr<Buffer>> m_ObjectConstantBuffers;
    std::unordered_map<const Model*, ModelResources> m_ModelCache;

    std::unique_ptr<Texture> m_DefaultDiffuse;
    std::unique_ptr<Texture> m_DefaultSpecular;
    std::unique_ptr<Texture> m_DefaultNormal;
    MaterialGPU m_DefaultMaterial{};

    bool m_VSync{false};
    bool m_TearingSupported{};
};

} // namespace GEngine
