#include "Playground.hpp"

#include "Core/Input.hpp"
#include "Core/Log.hpp"
#include "Core/Utility/MersenneTwister.hpp"
#include "Rendering/Components.hpp"
#include "Rendering/MeshFactory.hpp"
#include "Scene/Light.hpp"
#include "Scene/Material.hpp"
#include "Scene/Model.hpp"

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

    const std::filesystem::path containerTexturePath{"Assets\\Textures\\Container\\container2.png"};
    const GEngine::Material containerMaterial{
        .Albedo = {.Path = containerTexturePath,
                   .IsSRGB = true,
                   .Decoded = std::make_shared<GEngine::Image>(containerTexturePath)},
    };

    GEngine::AssetManager& assetManager = m_Scene.GetAssetManager();

    const GEngine::ModelHandle cubeModel = assetManager.AddModel(GEngine::Model{
        .Meshes = {GEngine::MeshFactory::Cube()},
        .Materials = {containerMaterial},
    });
    const GEngine::ModelHandle planeModel = assetManager.AddModel(GEngine::Model{
        .Meshes = {GEngine::MeshFactory::Plane()},
        .Materials = {containerMaterial},
    });

    const GEngine::ModelHandle porscheModel =
        m_Scene.GetAssetManager().LoadModel("Assets\\Models\\1975_porsche_911_930_turbo\\scene.gltf");
    const GEngine::ModelHandle sponzaModel =
        m_Scene.GetAssetManager().LoadModel("Assets\\Models\\Sponza\\glTF\\Sponza.gltf");

    auto& ecs = m_Scene.GetEntityRegistry();

    // Cubes at random positions
    using RNG = GEngine::MersenneTwister;
    RNG::Seed(42);
    for (uint32_t i{}; i < 10; ++i) {
        GEngine::Entity cube = ecs.Create();
        ecs.AddComponent<GEngine::Transform>(cube, GEngine::Transform{.Position = {
                                                                          RNG::GetRandom(-10.0f, 10.0f),
                                                                          RNG::GetRandom(0.5f, 10.0f),
                                                                          RNG::GetRandom(-10.0f, 10.0f),
                                                                      }});
        ecs.AddComponent<GEngine::ModelComponent>(cube, GEngine::ModelComponent{.Model = cubeModel});
    }

    // Ground plane
    GEngine::Entity plane = ecs.Create();
    ecs.AddComponent<GEngine::Transform>(plane, GEngine::Transform{.Scale = {500.0f, 1.0f, 500.0f}});
    ecs.AddComponent<GEngine::ModelComponent>(plane,
                                              GEngine::ModelComponent{.Model = planeModel, .CastsShadow = false});

    // Porsche
    GEngine::Entity porsche = ecs.Create();
    ecs.AddComponent<GEngine::Transform>(porsche, GEngine::Transform{});
    ecs.AddComponent<GEngine::ModelComponent>(porsche, GEngine::ModelComponent{.Model = porscheModel});

    // Sponza
    GEngine::Entity sponza = ecs.Create();
    ecs.AddComponent<GEngine::Transform>(sponza, GEngine::Transform{.Position = {20.0f, 5.0f, 20.0f}});
    ecs.AddComponent<GEngine::ModelComponent>(sponza, GEngine::ModelComponent{.Model = sponzaModel});

    auto skyboxTexture =
        m_Renderer.CreateTexture(GEngine::Image("Assets\\Textures\\Skybox\\citrus_orchard_road_puresky_4k.hdr"));
    m_Scene.SetSkybox(GEngine::Skybox{.Panorama = std::move(*skyboxTexture)});

    GE_INFO("Playground initialized successfully.");
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
