#pragma once

namespace GEngine {

class Window {
  public:
    Window(HINSTANCE hInstance, const std::wstring& title, uint32_t clientWidth, uint32_t clientHeight);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    [[nodiscard]] HWND GetHandle() const noexcept { return m_hWnd; }
    [[nodiscard]] uint32_t GetClientWidth() const noexcept { return m_ClientWidth; }
    [[nodiscard]] uint32_t GetClientHeight() const noexcept { return m_ClientHeight; }
    [[nodiscard]] bool IsRunning() const noexcept { return m_Running; }
    void Quit() noexcept { m_Running = false; }

    void SetFullscreen(bool fullscreen);
    void ToggleFullscreen() { SetFullscreen(!m_Fullscreen); }
    [[nodiscard]] bool IsFullscreen() const noexcept { return m_Fullscreen; }

    /// Runs the message loop. Calls onIdle each iteration (between WM_PAINT and next message).
    /// Returns the wParam from WM_QUIT.
    [[nodiscard]] int Run(const std::function<void()>& onIdle);

    /// Optional callback for application-level handling of window messages.
    /// Return 0 if the message was handled, non-zero to pass to default processing.
    using MessageCallback = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;
    void SetMessageCallback(MessageCallback callback) { m_MessageCallback = std::move(callback); }

    /// Resize callback — called when the window is resized (after updating internal state).
    using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
    void SetResizeCallback(ResizeCallback callback) { m_ResizeCallback = std::move(callback); }

  private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void RegisterWindowClass(HINSTANCE hInstance);
    void CreateAppWindow(HINSTANCE hInstance);

    HWND m_hWnd{};
    RECT m_WindowRect{};
    bool m_Running{true};
    uint32_t m_ClientWidth;
    uint32_t m_ClientHeight;
    bool m_Fullscreen{};
    std::wstring m_Title;
    std::wstring m_ClassName;

    MessageCallback m_MessageCallback;
    ResizeCallback m_ResizeCallback;
};

} // namespace GEngine
