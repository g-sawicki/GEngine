#include "PCH.hpp"

#include "SwapChain.hpp"

#include "Common.hpp"

using namespace Microsoft::WRL;

namespace GEngine {

bool SwapChain::CheckTearingSupport(IDXGIFactory5* factory) {
    BOOL allowTearing = FALSE;
    if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        return false;
    return allowTearing == TRUE;
}

SwapChain::SwapChain(IDXGIFactory5* factory, HWND hWnd, CommandQueue& commandQueue, uint32_t width, uint32_t height,
                     uint32_t bufferCount)
    : m_Width(width), m_Height(height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = width,
        .Height = height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Stereo = FALSE,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = bufferCount,
        .Scaling = DXGI_SCALING_NONE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = CheckTearingSupport(factory) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u,
    };

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(
        factory->CreateSwapChainForHwnd(commandQueue.GetHandle(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
    ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&m_SwapChain));

    RetrieveBackBuffers();
}

void SwapChain::OnResize(uint32_t width, uint32_t height) {
    m_Width = std::max(1u, width);
    m_Height = std::max(1u, height);

    for (uint32_t i = 0; i < NumFrames; ++i) {
        m_BackBuffers[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
    ThrowIfFailed(
        m_SwapChain->ResizeBuffers(NumFrames, m_Width, m_Height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    RetrieveBackBuffers();
}

void SwapChain::RetrieveBackBuffers() {
    for (uint32_t i = 0; i < NumFrames; ++i) {
        ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));
    }
}

HRESULT SwapChain::Present(UINT syncInterval, UINT flags) noexcept {
    return m_SwapChain->Present(syncInterval, flags);
}

} // namespace GEngine
