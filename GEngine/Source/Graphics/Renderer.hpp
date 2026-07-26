#pragma once

#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/SwapChain.hpp"

namespace GEngine {

class Renderer {
  public:
    struct FrameResource {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        uint64_t FenceValue{};
    };

    Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp);
    void Destroy();

    void Render();
    void OnResize(uint32_t width, uint32_t height);

    [[nodiscard]] bool IsDeviceRemoved() const noexcept { return m_Device && m_Device->IsDeviceRemoved(); }

    // Settings
    bool VSync{true};
    bool TearingSupported{};

  private:
    void UpdateRenderTargetViews();

    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    UINT m_RTVDescriptorSize{};
};

} // namespace GEngine
