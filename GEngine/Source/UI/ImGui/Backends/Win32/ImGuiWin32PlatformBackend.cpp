#include "PCH.hpp"

#include "ImGuiWin32PlatformBackend.hpp"

#include <backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace GEngine::UI {

ImGuiWin32PlatformBackend::ImGuiWin32PlatformBackend(void* nativeWindow) : m_NativeWindow(nativeWindow) {
    if (!m_NativeWindow)
        throw std::invalid_argument("The Win32 ImGui backend requires a valid window handle.");
}

ImGuiWin32PlatformBackend::~ImGuiWin32PlatformBackend() {
    if (m_Initialized)
        ImGui_ImplWin32_Shutdown();
}

void ImGuiWin32PlatformBackend::Initialize() {
    if (!ImGui_ImplWin32_Init(m_NativeWindow))
        throw std::runtime_error("Failed to initialize the ImGui Win32 backend.");
    m_Initialized = true;
}

bool ImGuiWin32PlatformBackend::HandleMessage(const WindowMessage& message) {
    return ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(message.Window), message.Message,
                                          static_cast<WPARAM>(message.WParam),
                                          static_cast<LPARAM>(message.LParam)) != 0;
}

void ImGuiWin32PlatformBackend::NewFrame() {
    ImGui_ImplWin32_NewFrame();
}

} // namespace GEngine::UI
