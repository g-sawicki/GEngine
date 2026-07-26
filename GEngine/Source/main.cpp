#include "PCH.hpp"

#include "Graphics/Renderer.hpp"
#include "Window.hpp"

using namespace GEngine;

class Application {
  public:
    void OnInit(HINSTANCE hInstance);
    void OnDestroy();
    int Run();

    void OnUpdate();
    void OnRender();
    void OnResize(uint32_t width, uint32_t height);

    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

  private:
    std::unique_ptr<Window> m_Window;
    Renderer m_Renderer;

    bool m_UseWarp{};

    // Timing
    uint64_t m_FrameCounter{0};
    double m_ElapsedSeconds{0.0};
    std::chrono::high_resolution_clock::time_point m_T0{std::chrono::high_resolution_clock::now()};
};

void Application::OnInit(HINSTANCE hInstance) {
    // Parse command line arguments.
    {
        int argc;
        wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

        for (int i{}; i < argc; ++i) {
            if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0) {
                m_UseWarp = true;
            }
        }

        ::LocalFree(argv);
    }

    // Create the window.
    m_Window = std::make_unique<Window>(hInstance, L"GEngine", 1280, 720);

    // Set up callbacks.
    m_Window->SetMessageCallback(
        [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) { return HandleMessage(hwnd, msg, wp, lp); });
    m_Window->SetResizeCallback([this](uint32_t width, uint32_t height) { OnResize(width, height); });

    m_Renderer.Init(m_Window->GetHandle(), m_Window->GetClientWidth(), m_Window->GetClientHeight(), m_UseWarp);
}

void Application::OnDestroy() {
    m_Renderer.Destroy();
}

int Application::Run() {
    return m_Window->Run([this]() {
        OnUpdate();
        OnRender();
    });
}

void Application::OnResize(uint32_t width, uint32_t height) {
    m_Renderer.OnResize(width, height);
}

void Application::OnUpdate() {
    ++m_FrameCounter;
    auto t1{std::chrono::high_resolution_clock::now()};
    auto deltaTime{t1 - m_T0};
    m_T0 = t1;
    m_ElapsedSeconds += std::chrono::duration<double>(deltaTime).count();
    if (m_ElapsedSeconds > 1.0) {
        auto fps{static_cast<double>(m_FrameCounter) / m_ElapsedSeconds};
        OutputDebugStringA(std::format("FPS: {:.1f}\n", fps).c_str());

        m_FrameCounter = 0;
        m_ElapsedSeconds = 0.0;
    }
}

void Application::OnRender() {
    m_Renderer.Render();

    // Check for device removal after present
    if (m_Renderer.IsDeviceRemoved()) {
        m_Window->Quit();
    }
}

LRESULT Application::HandleMessage([[maybe_unused]] HWND hwnd, UINT message, WPARAM wParam,
                                   [[maybe_unused]] LPARAM lParam) {
    switch (message) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt{(::GetAsyncKeyState(VK_MENU) & 0x8000) != 0};

        switch (wParam) {
        case 'V':
            m_Renderer.VSync = !m_Renderer.VSync;
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
    int result{app.Run()};
    app.OnDestroy();
    return result;
}
