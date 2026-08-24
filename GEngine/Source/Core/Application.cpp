#include "PCH.hpp"

#include "Application.hpp"
#include "CommandLine.hpp"

namespace GEngine {

Application::Application(const Specification& specification) : m_Specification(specification) {}

int Application::Run() {
    const HINSTANCE hInstance{::GetModuleHandle(nullptr)};

    std::vector<CommandLine::Argument> args = {
        {&m_UseWarp, L"--warp"},
    };
    CommandLine::Parse(args);

    // Create the window.
    m_Window =
        std::make_unique<Window>(hInstance, m_Specification.title, m_Specification.width, m_Specification.height);

    // Set up callbacks.
    m_Window->SetMessageCallback(
        [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) { return HandleMessage(hwnd, msg, wp, lp); });
    m_Window->SetResizeCallback([this](uint32_t width, uint32_t height) { HandleResize(width, height); });

    m_Renderer.Init(m_Window->GetHandle(), m_Window->GetClientWidth(), m_Window->GetClientHeight(), m_UseWarp,
                    m_Scene.GetShadowConfig().MapSize);

    OnInit();

    int result{m_Window->Run([this]() {
        // Delta time
        auto now{std::chrono::high_resolution_clock::now()};
        float deltaTime{std::min(std::chrono::duration<float>(now - m_PrevTime).count(), 0.25f)};
        m_PrevTime = now;

        // FPS counter
        ++m_FrameCounter;
        m_ElapsedSeconds += deltaTime;
        if (m_ElapsedSeconds > 1.0) {
            auto fps{static_cast<double>(m_FrameCounter) / m_ElapsedSeconds};
            OutputDebugStringA(std::format("FPS: {:.1f}\n", fps).c_str());

            m_FrameCounter = 0;
            m_ElapsedSeconds = 0.0;
        }

        OnUpdate(deltaTime);
        Render();

        // Check for device removal after present
        if (m_Renderer.IsDeviceRemoved()) {
            m_Window->Quit();
        }
    })};

    OnDestroy();
    return result;
}

void Application::OnInit() {}

void Application::OnDestroy() {
    m_Renderer.Destroy();
}

void Application::HandleResize(uint32_t width, uint32_t height) {
    m_Renderer.OnResize(width, height);

    const float aspectRatio = static_cast<float>(std::max(1u, width)) / static_cast<float>(std::max(1u, height));
    m_Scene.GetActiveCamera().SetAspectRatio(aspectRatio);
}

void Application::Render() {
    m_Renderer.Render(m_Scene);
}

LRESULT Application::HandleMessage([[maybe_unused]] HWND hwnd, UINT message, WPARAM wParam,
                                   [[maybe_unused]] LPARAM lParam) {
    switch (message) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        bool alt{(::GetAsyncKeyState(VK_MENU) & 0x8000) != 0};

        switch (wParam) {
        case 'V':
            m_Renderer.SetVSync(!m_Renderer.IsVSyncEnabled());
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

} // namespace GEngine
