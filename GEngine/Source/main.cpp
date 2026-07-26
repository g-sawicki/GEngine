#include "PCH.hpp"

#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Common.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/SwapChain.hpp"
#include "Window.hpp"

using namespace Microsoft::WRL;
using namespace GEngine;

class Application {
  public:
    void OnInit(HINSTANCE hInstance);
    void OnDestroy();
    int Run();

    void OnUpdate();
    void OnRender();
    void OnResize(uint32_t width, uint32_t height);

    void UpdateRenderTargetViews();
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

  public:
    struct FrameResource {
        ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        uint64_t FenceValue = 0;
    };

    // Window
    std::unique_ptr<Window> m_Window;

    // D3D12 wrappers
    std::unique_ptr<Device> m_Device;
    std::unique_ptr<CommandQueue> m_CommandQueue;
    std::unique_ptr<Fence> m_Fence;
    std::unique_ptr<SwapChain> m_SwapChain;

    FrameResource m_FrameResources[SwapChain::NumFrames]{};

    // Raw D3D12 objects
    ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    UINT m_RTVDescriptorSize{};

    // Settings
    bool m_UseWarp{};
    bool m_VSync{true};
    bool m_TearingSupported{};

    // Timing
    uint64_t m_FrameCounter{0};
    double m_ElapsedSeconds{0.0};
    std::chrono::high_resolution_clock::time_point m_T0{std::chrono::high_resolution_clock::now()};
};

static void ParseCommandLineArguments(Application& app) {
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (int i = 0; i < argc; ++i) {
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0) {
            app.m_UseWarp = true;
        }
    }

    ::LocalFree(argv);
}

static ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory6* dxgiFactory, bool useWarp) {
    if (useWarp) {
        ComPtr<IDXGIAdapter4> result;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&result)));
        return result;
    }

    // DXGI 1.6+: prefer the high-performance discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(
            dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)))) {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))) {
            ComPtr<IDXGIAdapter4> result;
            ThrowIfFailed(adapter.As(&result));
            return result;
        }
    }

    // Fallback: manually enumerate and pick the adapter with the most video memory
    SIZE_T maxDedicatedVideoMemory = 0;
    ComPtr<IDXGIAdapter4> selectedAdapter;
    for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
            desc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
            maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
            ThrowIfFailed(adapter.As(&selectedAdapter));
        }
    }

    return selectedAdapter;
}

static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                         uint32_t numDescriptors) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = numDescriptors,
    };

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Application::OnInit(HINSTANCE hInstance) {
    ParseCommandLineArguments(*this);

    // Create the window.
    m_Window = std::make_unique<Window>(hInstance, L"GEngine", 1280, 720);

    // Set up callbacks.
    m_Window->SetMessageCallback(
        [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) { return HandleMessage(hwnd, msg, wp, lp); });
    m_Window->SetResizeCallback([this](uint32_t width, uint32_t height) { OnResize(width, height); });

    Device::EnableDebugLayer();

    ComPtr<IDXGIFactory6> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(dxgiFactory.Get(), m_UseWarp);

    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());
    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        std::make_unique<SwapChain>(dxgiFactory.Get(), m_Window->GetHandle(), *m_CommandQueue,
                                    m_Window->GetClientWidth(), m_Window->GetClientHeight(), SwapChain::NumFrames);

    m_RTVDescriptorHeap = CreateDescriptorHeap(m_Device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChain::NumFrames);
    m_RTVDescriptorSize = m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews();

    for (uint32_t i = 0; i < SwapChain::NumFrames; ++i) {
        ThrowIfFailed(m_Device->Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              IID_PPV_ARGS(&m_FrameResources[i].CommandAllocator)));
        m_FrameResources[i].CommandList = std::make_unique<CommandList>(
            *m_Device, m_FrameResources[i].CommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    m_Fence = std::make_unique<Fence>(*m_Device);
}

void Application::OnDestroy() {
    m_Fence->Flush(m_CommandQueue->GetHandle());

    // Reset in reverse order of creation.
    for (auto& frame : m_FrameResources) {
        frame.CommandList.reset();
        frame.CommandAllocator.Reset();
    }
    m_Fence.reset();
    m_SwapChain.reset();
    m_CommandQueue.reset();
    m_RTVDescriptorHeap.Reset();
    m_Device.reset();

#if defined(_DEBUG)
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                         DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif
}

int Application::Run() {
    return m_Window->Run([this]() {
        OnUpdate();
        OnRender();
    });
}

void Application::UpdateRenderTargetViews() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < SwapChain::NumFrames; ++i) {
        ID3D12Resource* backBuffer = m_SwapChain->GetBackBuffer(i);
        m_Device->Get()->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
        rtvHandle.Offset(m_RTVDescriptorSize);
    }
}

void Application::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence->Flush(m_CommandQueue->GetHandle());
    m_SwapChain->OnResize(width, height);

    UpdateRenderTargetViews();
}

void Application::OnUpdate() {
    ++m_FrameCounter;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto deltaTime = t1 - m_T0;
    m_T0 = t1;
    m_ElapsedSeconds += std::chrono::duration<double>(deltaTime).count();
    if (m_ElapsedSeconds > 1.0) {
        auto fps = static_cast<double>(m_FrameCounter) / m_ElapsedSeconds;
        OutputDebugStringA(std::format("FPS: {:.1f}\n", fps).c_str());

        m_FrameCounter = 0;
        m_ElapsedSeconds = 0.0;
    }
}

void Application::OnRender() {
    auto currentIdx = m_SwapChain->GetCurrentBackBufferIndex();
    auto& frame = m_FrameResources[currentIdx];

    // Ensure the GPU has finished with this frame's resources before reusing them.
    m_Fence->WaitForValue(frame.FenceValue);

    ID3D12Resource* backBuffer = m_SwapChain->GetCurrentBackBuffer();

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList = frame.CommandList->GetHandle();

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        cmdList->ResourceBarrier(1, &barrier);
        static constexpr FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentIdx,
                                          m_RTVDescriptorSize);

        cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        cmdList->ResourceBarrier(1, &barrier);
        frame.CommandList->Close();

        ID3D12CommandList* const ppCommandLists[] = {cmdList};
        m_CommandQueue->ExecuteCommandLists(ppCommandLists);

        UINT syncInterval = m_VSync ? 1u : 0u;
        UINT presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

        frame.FenceValue = m_Fence->Signal(m_CommandQueue->GetHandle());

        // Check for device removal after present
        if (m_Device->IsDeviceRemoved()) {
            m_Window->Quit();
        }
    }
}

LRESULT Application::HandleMessage([[maybe_unused]] HWND hwnd, UINT message, WPARAM wParam,
                                   [[maybe_unused]] LPARAM lParam) {
    switch (message) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam) {
        case 'V':
            m_VSync = !m_VSync;
            return 0;
        case VK_ESCAPE:
            m_Window->Quit();
            return 0;
        case VK_RETURN:
            if (alt)
                m_Window->ToggleFullscreen();
            return 0;
        case VK_F11:
            m_Window->ToggleFullscreen();
            return 0;
        }
    } break;
    case WM_SYSCHAR:
        return 0;
    }

    // Signal that the message was not handled — Window will forward to DefWindowProc.
    return -1;
}

int CALLBACK wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] PWSTR lpCmdLine,
                      [[maybe_unused]] int nCmdShow) {
    Application app{};
    app.OnInit(hInstance);
    int result = app.Run();
    app.OnDestroy();
    return result;
}
