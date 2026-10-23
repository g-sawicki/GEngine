#include "PCH.hpp"

#include "ImGuiLayer.hpp"

#include <imgui.h>

namespace {

void DestroyImGuiContext(ImGuiContext* context) {
    if (!context)
        return;
    ImGui::SetCurrentContext(context);
    ImGui::DestroyContext(context);
}

} // namespace

namespace GEngine::UI {

ImGuiLayer::ImGuiLayer(std::unique_ptr<ImGuiPlatformBackend> platformBackend,
                       std::unique_ptr<ImGuiRendererBackend> rendererBackend)
    : m_Context(nullptr, &DestroyImGuiContext), m_PlatformBackend(std::move(platformBackend)),
      m_RendererBackend(std::move(rendererBackend)) {
    if (!m_PlatformBackend || !m_RendererBackend)
        throw std::invalid_argument("ImGui requires both a platform backend and a renderer backend.");

    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    if (!context)
        throw std::runtime_error("Failed to create the ImGui context.");
    m_Context.reset(context);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    m_PlatformBackend->Initialize();
    m_RendererBackend->Initialize();
}

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

void ImGuiLayer::BeginFrame() {
    ImGui::SetCurrentContext(m_Context.get());
    m_PlatformBackend->NewFrame();
    m_RendererBackend->NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

bool ImGuiLayer::HandleMessage(const WindowMessage& message) {
    ImGui::SetCurrentContext(m_Context.get());
    return m_PlatformBackend->HandleMessage(message);
}

bool ImGuiLayer::WantsMouseCapture() const noexcept {
    if (!m_Context)
        return false;
    ImGui::SetCurrentContext(m_Context.get());
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantsKeyboardCapture() const noexcept {
    if (!m_Context)
        return false;
    ImGui::SetCurrentContext(m_Context.get());
    return ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiLayer::Render(void* nativeCommandList) {
    ImGui::SetCurrentContext(m_Context.get());
    ImGui::Render();
    m_RendererBackend->Render(ImGui::GetDrawData(), nativeCommandList);
}

void ImGuiLayer::Shutdown() noexcept {
    if (!m_Context)
        return;

    ImGui::SetCurrentContext(m_Context.get());
    m_RendererBackend.reset();
    m_PlatformBackend.reset();
    m_Context.reset();
}

} // namespace GEngine::UI
