#pragma once

#include "ImGuiBackend.hpp"

#include <memory>

struct ImGuiContext;

namespace GEngine::UI {

class ImGuiLayer final : public Overlay {
  public:
    ImGuiLayer(std::unique_ptr<ImGuiPlatformBackend> platformBackend,
               std::unique_ptr<ImGuiRendererBackend> rendererBackend);
    ~ImGuiLayer() override;

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
    ImGuiLayer(ImGuiLayer&&) = delete;
    ImGuiLayer& operator=(ImGuiLayer&&) = delete;

    void BeginFrame() override;
    bool HandleMessage(const WindowMessage& message) override;
    [[nodiscard]] bool WantsMouseCapture() const noexcept override;
    [[nodiscard]] bool WantsKeyboardCapture() const noexcept override;
    void Render(void* nativeCommandList) override;
    void Shutdown() noexcept override;

  private:
    using ContextPtr = std::unique_ptr<ImGuiContext, void (*)(ImGuiContext*)>;

    ContextPtr m_Context;
    std::unique_ptr<ImGuiPlatformBackend> m_PlatformBackend;
    std::unique_ptr<ImGuiRendererBackend> m_RendererBackend;
};

} // namespace GEngine::UI
