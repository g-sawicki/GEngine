#pragma once

#include "UI/ImGui/ImGuiBackend.hpp"

namespace GEngine::UI {

class ImGuiWin32PlatformBackend final : public ImGuiPlatformBackend {
  public:
    explicit ImGuiWin32PlatformBackend(void* nativeWindow);
    ~ImGuiWin32PlatformBackend() override;

    void Initialize() override;
    bool HandleMessage(const WindowMessage& message) override;
    void NewFrame() override;

  private:
    void* m_NativeWindow{};
    bool m_Initialized{};
};

} // namespace GEngine::UI
