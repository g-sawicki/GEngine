#pragma once

#include "CommandQueue.hpp"
#include "Texture.hpp"

namespace GEngine {

class SwapChain {
  public:
    static bool CheckTearingSupport(IDXGIFactory5* factory);
    static constexpr uint32_t NumFrames{3u};
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

  public:
    SwapChain(IDXGIFactory5* factory, HWND hWnd, CommandQueue& commandQueue, uint32_t width, uint32_t height,
              uint32_t bufferCount);

    [[nodiscard]] IDXGISwapChain4* GetHandle() const noexcept { return m_SwapChain.Get(); }
    [[nodiscard]] UINT GetCurrentBackBufferIndex() const noexcept { return m_SwapChain->GetCurrentBackBufferIndex(); }
    [[nodiscard]] Texture& GetCurrentBackBuffer() noexcept { return m_BackBuffers[GetCurrentBackBufferIndex()]; }
    [[nodiscard]] const Texture& GetCurrentBackBuffer() const noexcept {
      return m_BackBuffers[GetCurrentBackBufferIndex()];
    }
    [[nodiscard]] const Texture& GetBackBuffer(UINT index) const noexcept { return m_BackBuffers[index]; }
    [[nodiscard]] uint32_t GetWidth() const noexcept { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }

    HRESULT Present(UINT syncInterval, UINT flags) noexcept;
    void OnResize(uint32_t width, uint32_t height);

  private:
    void RetrieveBackBuffers();

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
    std::array<Texture, NumFrames> m_BackBuffers;
    uint32_t m_Width{};
    uint32_t m_Height{};
};

} // namespace GEngine
