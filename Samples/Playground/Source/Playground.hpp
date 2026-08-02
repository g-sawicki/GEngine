#pragma once

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Scene/CameraController.hpp"

class Playground : public GEngine::Application {
  public:
    using Application::Application;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

  private:
    void UpdateCamera(float deltaTime);

    std::unique_ptr<GEngine::CameraController> m_CameraController;
    std::unique_ptr<GEngine::MeshBuffer> m_Cube;

    // Mouse tracking
    POINT m_PrevCursorPos{};
    bool m_MouseCaptured{};
};
