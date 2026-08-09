#pragma once

#include "Core/Application.hpp"
#include "Scene/CameraController.hpp"

class Playground final : public GEngine::Application {
  public:
    explicit Playground(Specification specification);

    void OnInit() override;
    void OnUpdate(float deltaTime) override;

  private:
    void UpdateCamera(float deltaTime);
    void UpdateShadowCubePosition();

    GEngine::CameraController m_CameraController{};
    GEngine::EntityId m_ShadowCube{};

    // Mouse tracking
    POINT m_PrevCursorPos{};
    bool m_MouseCaptured{};
};
