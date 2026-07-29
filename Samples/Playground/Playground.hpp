#pragma once

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Scene/CameraController.hpp"

class Playground : public GEngine::Application {
  public:
    using Application::Application;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;

  private:
    void UpdateCamera(float deltaTime);

    std::unique_ptr<GEngine::CameraController> m_CameraController;

    // Mouse tracking
    POINT m_PrevCursorPos{};
    bool m_MouseCaptured{};
};
