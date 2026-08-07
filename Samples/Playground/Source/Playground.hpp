#pragma once

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/Utility/Image.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Scene/CameraController.hpp"

#include <vector>

class Playground : public GEngine::Application {
  public:
    using Application::Application;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

  private:
    void UpdateCamera(float deltaTime);

    std::unique_ptr<GEngine::CameraController> m_CameraController;
    std::unique_ptr<GEngine::MeshBuffer> m_CubeMesh;
    std::unique_ptr<GEngine::MeshBuffer> m_PlaneMesh;
    std::vector<std::unique_ptr<GEngine::Buffer>> m_CubeObjectCBs;
    std::unique_ptr<GEngine::Buffer> m_PlaneObjectCB;

    std::unique_ptr<GEngine::Texture> m_ContainerDiffuseTex;
    std::unique_ptr<GEngine::Texture> m_ContainerSpecularTex;

    // Mouse tracking
    POINT m_PrevCursorPos{};
    bool m_MouseCaptured{};
};
