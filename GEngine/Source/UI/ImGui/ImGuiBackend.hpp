#pragma once

#include "Core/UI/UiOverlay.hpp"

struct ImDrawData;

namespace GEngine::UI {

class ImGuiPlatformBackend {
  public:
    virtual ~ImGuiPlatformBackend() = default;

    virtual void Initialize() = 0;
    virtual bool HandleMessage(const WindowMessage& message) = 0;
    virtual void NewFrame() = 0;
};

class ImGuiRendererBackend {
  public:
    virtual ~ImGuiRendererBackend() = default;

    virtual void Initialize() = 0;
    virtual void NewFrame() = 0;
    virtual void Render(ImDrawData* drawData, void* nativeCommandList) = 0;
};

} // namespace GEngine::UI
