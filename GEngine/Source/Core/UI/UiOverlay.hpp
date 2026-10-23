#pragma once

#include <cstdint>

namespace GEngine::UI {

struct WindowMessage {
    void* Window{};
    uint32_t Message{};
    uintptr_t WParam{};
    intptr_t LParam{};
};

class Overlay {
  public:
    virtual ~Overlay() = default;

    virtual void BeginFrame() = 0;
    [[nodiscard]] virtual bool HandleMessage(const WindowMessage& message) = 0;
    [[nodiscard]] virtual bool WantsMouseCapture() const noexcept = 0;
    [[nodiscard]] virtual bool WantsKeyboardCapture() const noexcept = 0;
    virtual void Render(void* nativeCommandList) = 0;
    virtual void Shutdown() noexcept = 0;
};

} // namespace GEngine::UI
