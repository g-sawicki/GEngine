#include "PCH.hpp"

#include "Graphics/Renderer.hpp"
#include "Scene/CameraController.hpp"
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
    CameraController m_CameraController{
        {.FovDegrees = 70.0f, .AspectRatio = 1280.0f / 720.0f, .NearZ = 0.1f, .FarZ = 1000.0f}, {0.0f, 0.0f, -5.0f}};

    bool m_UseWarp{};

    // Timing
    uint64_t m_FrameCounter{0};
    double m_ElapsedSeconds{0.0};
    std::chrono::high_resolution_clock::time_point m_PrevTime{std::chrono::high_resolution_clock::now()};
    float m_DeltaTime{};

    // Mouse tracking
    POINT m_PrevCursorPos{};
    bool m_MouseCaptured{};
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
    const float aspectRatio = static_cast<float>(std::max(1u, width)) / static_cast<float>(std::max(1u, height));
    m_CameraController.GetCamera().SetAspectRatio(aspectRatio);
}

void Application::OnUpdate() {
    // Delta time
    auto now{std::chrono::high_resolution_clock::now()};
    m_DeltaTime = std::min(std::chrono::duration<float>(now - m_PrevTime).count(), 0.25f);
    m_PrevTime = now;

    // FPS counter
    ++m_FrameCounter;
    m_ElapsedSeconds += m_DeltaTime;
    if (m_ElapsedSeconds > 1.0) {
        auto fps{static_cast<double>(m_FrameCounter) / m_ElapsedSeconds};
        OutputDebugStringA(std::format("FPS: {:.1f}\n", fps).c_str());

        m_FrameCounter = 0;
        m_ElapsedSeconds = 0.0;
    }

    // Build camera input from keyboard and mouse.
    CameraInput input;
    input.MoveForward = (::GetAsyncKeyState('W') & 0x8000) != 0;
    input.MoveBack = (::GetAsyncKeyState('S') & 0x8000) != 0;
    input.MoveRight = (::GetAsyncKeyState('D') & 0x8000) != 0;
    input.MoveLeft = (::GetAsyncKeyState('A') & 0x8000) != 0;
    input.MoveUp = (::GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    input.MoveDown = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    // Mouse delta: right-click toggles mouse look.
    {
        bool rightDown{(::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0};

        if (rightDown && !m_MouseCaptured) {
            m_MouseCaptured = true;
            ::GetCursorPos(&m_PrevCursorPos);
            ::ShowCursor(FALSE);
        } else if (!rightDown && m_MouseCaptured) {
            m_MouseCaptured = false;
            ::ShowCursor(TRUE);
        }

        if (m_MouseCaptured) {
            POINT cursorPos;
            ::GetCursorPos(&cursorPos);
            input.MouseDeltaX = static_cast<float>(cursorPos.x - m_PrevCursorPos.x);
            input.MouseDeltaY = static_cast<float>(cursorPos.y - m_PrevCursorPos.y);
            ::SetCursorPos(m_PrevCursorPos.x, m_PrevCursorPos.y);
        }
    }

    m_CameraController.Update(m_DeltaTime, input);
}

void Application::OnRender() {
    const auto& camera{m_CameraController.GetCamera()};
    const DirectX::XMMATRIX view{camera.GetViewMatrix()};
    const DirectX::XMMATRIX proj{camera.GetProjectionMatrix()};
    m_Renderer.Render(DirectX::XMMatrixMultiply(view, proj));

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
        case VK_RBUTTON:
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
