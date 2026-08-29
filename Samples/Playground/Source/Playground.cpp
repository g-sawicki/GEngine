#include "Playground.hpp"

#include "Core/Input.hpp"
#include "Core/Utility/MersenneTwister.hpp"
#include "Rendering/MeshFactory.hpp"
#include "Scene/Light.hpp"
#include "Scene/Material.hpp"
#include "Scene/Model.hpp"
#include "Scene/Transform.hpp"

#include <cmath>

using namespace DirectX;

Playground::Playground(Specification specification) : Application(specification) {
    GEngine::Camera& camera = m_Scene.CreateCamera({
        .FovDegrees = 70.0f,
        .AspectRatio = static_cast<float>(specification.width) / static_cast<float>(specification.height),
        .NearZ = 0.1f,
        .FarZ = 1000.0f,
    });
    camera.SetPosition({20.0f, 6.0f, 20.0f});
    m_CameraController = GEngine::CameraController(&camera);
}

void Playground::OnInit() {
    XMFLOAT3 lightDirection;
    XMStoreFloat3(&lightDirection, XMVector3Normalize(XMVectorSet(1.0f, -4.0f, 2.0f, 0.0f)));
    m_Scene.SetDirectionalLight({
        .Direction = lightDirection,
        .Intensity = 1.0f,
        .Color = {1.0f, 1.0f, 1.0f},
    });

    m_Scene.AddPointLight({
        .Position = {10.0f, 7.0f, 20.0f},
        .Intensity = 50.0f,
        .Color = {1.0f, 0.6f, 0.3f},
    });

    m_Scene.AddSpotLight({
        .Position = {30.0f, 7.0f, 20.0f},
        .Intensity = 10.0f,
        .Direction = {0.0f, -1.0f, 0.0f},
        .Color = {3.0f, 0.6f, 1.0f},
        .InnerConeAngle = 45.0f,
        .OuterConeAngle = 60.0f,
    });

    const GEngine::Material containerMaterial{
        .Albedo = std::make_shared<GEngine::Image>(GEngine::Image{"Assets\\Textures\\Container\\container2.png"}),
    };

    const auto cubeModel = m_Scene.AddModel(GEngine::MeshFactory::Cube(), containerMaterial);
    const auto planeModel = m_Scene.AddModel(GEngine::MeshFactory::Plane(), containerMaterial);
    const auto porscheModel = m_Scene.LoadModel("Assets\\Models\\1975_porsche_911_930_turbo\\scene.gltf");
    const auto sponzaModel = m_Scene.LoadModel("Assets\\Models\\Sponza\\glTF\\Sponza.gltf");

    // Cubes at random positions
    using RNG = GEngine::MersenneTwister;
    RNG::Seed(42);
    for (uint32_t i{}; i < 10; ++i) {
        m_Scene.GetEntityManager().SpawnEntity(cubeModel, GEngine::Transform::FromPosition({
                                                              RNG::GetRandom(-10.0f, 10.0f),
                                                              RNG::GetRandom(0.5f, 10.0f),
                                                              RNG::GetRandom(-10.0f, 10.0f),
                                                          }));
    }

    // Ground plane
    m_Scene.GetEntityManager().SpawnEntity(planeModel, GEngine::Transform::FromScale({500.0f, 1.0f, 500.0f}), false);

    // Porsche
    m_Scene.GetEntityManager().SpawnEntity(porscheModel);

    // Sponza
    m_Scene.GetEntityManager().SpawnEntity(sponzaModel, GEngine::Transform{.Position = {20.0f, 5.0f, 20.0f}});

    auto skyboxTexture =
        m_Renderer.CreateTexture(GEngine::Image("Assets\\Textures\\Skybox\\citrus_orchard_road_puresky_4k.hdr"));
    m_Scene.SetSkybox(GEngine::Skybox{.Panorama = std::move(*skyboxTexture)});
}

void Playground::OnUpdate(float deltaTime) {
    UpdateCamera(deltaTime);
}

void Playground::UpdateCamera(float deltaTime) {
    // Build camera input from keyboard and mouse.
    GEngine::CameraInput input;
    input.MoveForward = GEngine::Input::IsKeyDown('W');
    input.MoveBack = GEngine::Input::IsKeyDown('S');
    input.MoveRight = GEngine::Input::IsKeyDown('D');
    input.MoveLeft = GEngine::Input::IsKeyDown('A');
    input.MoveUp = GEngine::Input::IsKeyDown(VK_SPACE);
    input.MoveDown = GEngine::Input::IsKeyDown(VK_SHIFT);

    // Mouse delta: right-click toggles mouse look.
    {
        bool rightDown{GEngine::Input::IsKeyDown(VK_RBUTTON)};

        if (rightDown && !m_MouseCaptured) {
            m_MouseCaptured = true;
            m_PrevCursorPos = GEngine::Input::GetCursorPosition();
            GEngine::Input::HideCursor();
        } else if (!rightDown && m_MouseCaptured) {
            m_MouseCaptured = false;
            GEngine::Input::ShowCursor(true);
        }

        if (m_MouseCaptured) {
            POINT cursorPos{GEngine::Input::GetCursorPosition()};
            input.MouseDeltaX = static_cast<float>(cursorPos.x - m_PrevCursorPos.x);
            input.MouseDeltaY = static_cast<float>(cursorPos.y - m_PrevCursorPos.y);
            GEngine::Input::SetCursorPosition(m_PrevCursorPos.x, m_PrevCursorPos.y);
        }
    }

    m_CameraController.Update(deltaTime, input);
}
