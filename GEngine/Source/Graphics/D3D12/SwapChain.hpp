#pragma once

#include "CommandQueue.hpp"
#include "Core/Utility/Defines.hpp"
#include "Texture.hpp"

namespace GEngine {

class SwapChain {
  public:
    static constexpr uint32_t NumFrames{3u};
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

  public:
    SwapChain(Device& device, HWND hWnd, CommandQueue& commandQueue, uint32_t width, uint32_t height,
              uint32_t bufferCount);

    GE_NO_COPY_DEFAULT_MOVE(SwapChain)

    [[nodiscard]] IDXGISwapChain4* GetHandle() const noexcept { return m_SwapChain.Get(); }
    [[nodiscard]] UINT GetCurrentBackBufferIndex() const noexcept { return m_SwapChain->GetCurrentBackBufferIndex(); }
    [[nodiscard]] Texture& GetCurrentBackBuffer() noexcept { return m_BackBuffers[GetCurrentBackBufferIndex()]; }
    [[nodiscard]] const Texture& GetCurrentBackBuffer() const noexcept {
        return m_BackBuffers[GetCurrentBackBufferIndex()];
    }
    [[nodiscard]] const Texture& GetBackBuffer(UINT index) const noexcept { return m_BackBuffers[index]; }
    [[nodiscard]] uint32_t GetWidth() const noexcept { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }
    [[nodiscard]] bool IsTearingSupported() const noexcept { return m_TearingSupported; }

    void SetVSync(bool enabled) noexcept { m_VSync = enabled; }
    [[nodiscard]] bool IsVSyncEnabled() const noexcept { return m_VSync; }

    HRESULT Present() noexcept;
    void OnResize(uint32_t width, uint32_t height);

  private:
    static bool CheckTearingSupport(IDXGIFactory5* factory);

    void RetrieveBackBuffers();

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
    std::array<Texture, NumFrames> m_BackBuffers;
    uint32_t m_Width{};
    uint32_t m_Height{};
    bool m_TearingSupported{};
    bool m_VSync{};
};

} // namespace GEngine
