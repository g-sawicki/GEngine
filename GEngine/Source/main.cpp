#include "PCH.hpp"

#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Common.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/SwapChain.hpp"

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
    void OnSetFullscreen(bool fullscreen);

    void UpdateRenderTargetViews();

  public:
    // Window
    HWND m_hWnd{};
    RECT m_WindowRect{};
    bool m_Running{true};

    struct FrameResource {
        ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<CommandList> CommandList;
        uint64_t FenceValue = 0;
    };

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
    uint32_t m_ClientWidth{1280};
    uint32_t m_ClientHeight{720};
    bool m_UseWarp{};
    bool m_VSync{true};
    bool m_TearingSupported{};
    bool m_Fullscreen{};

    // Timing
    uint64_t m_FrameCounter{0};
    double m_ElapsedSeconds{0.0};
    std::chrono::high_resolution_clock::time_point m_T0{std::chrono::high_resolution_clock::now()};
};

// ---------------------------------------------------------------------------
// Free helpers
// ---------------------------------------------------------------------------

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

static HWND CreateAppWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width,
                            uint32_t height) {
    const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    const int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    const int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hWnd = ::CreateWindowExW(NULL, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, windowX, windowY,
                                  windowWidth, windowHeight, NULL, NULL, hInst, nullptr);

    assert(hWnd && "Failed to create window");

    return hWnd;
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

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName) {
    WNDCLASSEXW windowClass = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = &WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInst,
        .hIcon = ::LoadIcon(hInst, NULL),
        .hCursor = ::LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName = NULL,
        .lpszClassName = windowClassName,
        .hIconSm = ::LoadIcon(hInst, NULL),
    };

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

void Application::OnInit(HINSTANCE hInstance) {
    ParseCommandLineArguments(*this);

    Device::EnableDebugLayer();

    ComPtr<IDXGIFactory6> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    const wchar_t* windowClassName = L"DX12WindowClass";
    RegisterWindowClass(hInstance, windowClassName);
    m_hWnd = CreateAppWindow(windowClassName, hInstance, L"Learning DirectX 12", m_ClientWidth, m_ClientHeight);

    // Stash the Application pointer so WndProc can access it.
    ::SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    ::GetWindowRect(m_hWnd, &m_WindowRect);

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(dxgiFactory.Get(), m_UseWarp);

    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());
    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain = std::make_unique<SwapChain>(dxgiFactory.Get(), m_hWnd, *m_CommandQueue, m_ClientWidth, m_ClientHeight,
                                              SwapChain::NumFrames);

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
    ::ShowWindow(m_hWnd, SW_SHOW);

    MSG msg{};
    while (m_Running) {
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                m_Running = false;
                break;
            }
        }

        if (!m_Running)
            break;

        OnUpdate();
        OnRender();
    }

    OnDestroy();
    return static_cast<int>(msg.wParam);
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
    if (m_ClientWidth != width || m_ClientHeight != height) {
        m_ClientWidth = std::max(1u, width);
        m_ClientHeight = std::max(1u, height);

        m_Fence->Flush(m_CommandQueue->GetHandle());
        m_SwapChain->OnResize(m_ClientWidth, m_ClientHeight);

        UpdateRenderTargetViews();
    }
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
            m_Running = false;
        }
    }
}

void Application::OnSetFullscreen(bool enableFullscreen) {
    if (m_Fullscreen == enableFullscreen)
        return;

    m_Fullscreen = enableFullscreen;

    if (m_Fullscreen) {
        ::GetWindowRect(m_hWnd, &m_WindowRect);
        UINT windowStyle =
            WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);
        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo{};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &monitorInfo);
        ::SetWindowPos(m_hWnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                       monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                       monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_MAXIMIZE);
    } else {
        ::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_WindowRect.left, m_WindowRect.top,
                       m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top,
                       SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_NORMAL);
    }

    // Ensure swap chain buffers match the new window size
    RECT clientRect{};
    ::GetClientRect(m_hWnd, &clientRect);
    OnResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto app = reinterpret_cast<Application*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!app)
        return ::DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
    case WM_PAINT:
        ::ValidateRect(hwnd, nullptr);
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam) {
        case 'V':
            app->m_VSync = !app->m_VSync;
            break;
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        case VK_RETURN:
            if (alt)
                app->OnSetFullscreen(!app->m_Fullscreen);
            break;
        case VK_F11:
            app->OnSetFullscreen(!app->m_Fullscreen);
            break;
        }
    } break;
    case WM_SYSCHAR:
        break;
    case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED)
            break;

        RECT clientRect{};
        ::GetClientRect(hwnd, &clientRect);

        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        app->OnResize(width, height);
    } break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

int CALLBACK wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] PWSTR lpCmdLine,
                      [[maybe_unused]] int nCmdShow) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Application app;
    app.OnInit(hInstance);
    return app.Run();
}
