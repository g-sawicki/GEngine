#pragma once

#include "Core/World.hpp"

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/SwapChain.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Rendering/RenderItem.hpp"
#include "Rendering/RenderPass/ForwardLighting.hpp"

namespace GEngine {

class Renderer {
  public:
    struct FrameResource {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        std::unique_ptr<Buffer> SceneInfoConstantBuffer;
        uint64_t FenceValue{};
    };

    Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp);
    void Destroy();

    void Render(const SceneInfo& sceneInfo);
    void OnResize(uint32_t width, uint32_t height);

    std::unique_ptr<MeshBuffer> CreateMeshBuffer(const Mesh& mesh);
    std::unique_ptr<Buffer> CreateConstantBuffer(UINT64 size);
    void DrawMesh(const MeshBuffer& mesh, const Buffer& objectCB);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device && m_Device->IsDeviceRemoved(); }

    void SetVSync(bool enabled) noexcept { m_VSync = enabled; }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_VSync; }

  private:
    void UpdateRenderTargetViews();

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    UINT m_RTVDescriptorSize{};

    std::unique_ptr<RenderPass::ForwardLighting> m_ForwardLighting;
    std::vector<RenderItem> m_RenderItems;

    bool m_VSync{true};
    bool m_TearingSupported{};
};

} // namespace GEngine
