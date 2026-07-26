#include "PCH.hpp"

#include "Window.hpp"

namespace GEngine {

Window::Window(HINSTANCE hInstance, const std::wstring& title, uint32_t clientWidth, uint32_t clientHeight)
    : m_ClientWidth{clientWidth}, m_ClientHeight{clientHeight}, m_Title{title}, m_ClassName{L"GEngineWindowClass"} {
    ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    RegisterWindowClass(hInstance);
    CreateAppWindow(hInstance);

    // Stash the Window pointer so WndProc can access it.
    ::SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

Window::~Window() {
    if (m_hWnd) {
        ::DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

void Window::RegisterWindowClass(HINSTANCE hInstance) {
    static bool classRegistered = false;
    if (classRegistered)
        return;

    WNDCLASSEXW windowClass = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = &Window::WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = ::LoadIcon(hInstance, nullptr),
        .hCursor = ::LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        .lpszMenuName = nullptr,
        .lpszClassName = m_ClassName.c_str(),
        .hIconSm = ::LoadIcon(hInstance, nullptr),
    };

    ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
    classRegistered = true;
}

void Window::CreateAppWindow(HINSTANCE hInstance) {
    const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = {0, 0, static_cast<LONG>(m_ClientWidth), static_cast<LONG>(m_ClientHeight)};
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen.
    const int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    const int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    m_hWnd = ::CreateWindowExW(0, m_ClassName.c_str(), m_Title.c_str(), WS_OVERLAPPEDWINDOW, windowX, windowY,
                               windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    assert(m_hWnd && "Failed to create window");
}

int Window::Run(const std::function<void()>& onIdle) {
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

        if (onIdle)
            onIdle();
    }

    return static_cast<int>(msg.wParam);
}

void Window::SetFullscreen(bool enableFullscreen) {
    if (m_Fullscreen == enableFullscreen)
        return;

    m_Fullscreen = enableFullscreen;

    if (m_Fullscreen) {
        ::GetWindowRect(m_hWnd, &m_WindowRect);
        UINT windowStyle =
            WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);
        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{
            .cbSize = sizeof(MONITORINFO),
        };
        ::GetMonitorInfo(hMonitor, &monitorInfo);
        ::SetWindowPos(m_hWnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                       monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                       monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_MAXIMIZE);
    } else {
        ::SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_WindowRect.left, m_WindowRect.top,
                       m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top,
                       SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_NORMAL);
    }

    // Notify the application of the new client size so it can resize resources.
    if (m_ResizeCallback) {
        RECT clientRect{};
        ::GetClientRect(m_hWnd, &clientRect);
        m_ResizeCallback(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    }
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* window = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!window)
        return ::DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
    case WM_PAINT:
        ::ValidateRect(hwnd, nullptr);
        break;

    case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED)
            break;

        RECT clientRect{};
        ::GetClientRect(hwnd, &clientRect);
        window->m_ClientWidth = clientRect.right - clientRect.left;
        window->m_ClientHeight = clientRect.bottom - clientRect.top;

        if (window->m_ResizeCallback)
            window->m_ResizeCallback(window->m_ClientWidth, window->m_ClientHeight);
    } break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;

    default:
        // Forward unhandled messages to the application callback.
        if (window->m_MessageCallback) {
            LRESULT result = window->m_MessageCallback(hwnd, message, wParam, lParam);
            if (result == 0)
                return 0;
        }
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

} // namespace GEngine
