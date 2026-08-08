#include "Playground.hpp"

#include "Core/Utility/MersenneTwister.hpp"
#include "Rendering/MeshFactory.hpp"
#include "Scene/Light.hpp"

#include <cmath>

using namespace DirectX;

void Playground::OnInit() {
    auto& world = GetWorld();

    auto& camera = world.CreateCamera(
        {.FovDegrees = 70.0f, .AspectRatio = 1280.0f / 720.0f, .NearZ = 0.1f, .FarZ = 1000.0f}, {0.0f, 6.0f, -12.0f});
    camera.SetRotation(DirectX::XMConvertToRadians(-25.0f), 0.0f);
    m_CameraController = std::make_unique<GEngine::CameraController>(camera);

    GEngine::DirectionalLight directionalLight{
        .Intensity = 1.0f,
        .Color = {1.0f, 1.0f, 1.0f},
    };
    XMStoreFloat3(&directionalLight.Direction, XMVector3Normalize(XMVectorSet(1.0f, -4.0f, 2.0f, 0.0f)));
    world.SetDirectionalLight(directionalLight);

    world.SetShadowConfig({
        .Enabled = true,
        .MapSize = 1024,
        .Bias = 0.005f,
        .SlopeScaleBias = 2.0f,
        .NormalOffsetScale = 1.0f,
        .NearZ = 0.1f,
        .FarZ = 100.0f,
    });

    GEngine::Image containerDiffuseMap{"Assets\\Textures\\Container\\container2.png"};
    GEngine::Image containerSpecularMap{"Assets\\Textures\\Container\\container2_specular.png"};

    auto& renderer = GetRenderer();
    m_ContainerDiffuseTex = renderer.CreateTexture(containerDiffuseMap);
    m_ContainerSpecularTex = renderer.CreateTexture(containerSpecularMap);
    m_CubeMesh = renderer.CreateMeshBuffer(GEngine::MeshFactory::Cube());
    m_PlaneMesh = renderer.CreateMeshBuffer(GEngine::MeshFactory::Plane());

    m_PlaneObjectCB = renderer.CreateConstantBuffer(sizeof(XMFLOAT4X4));
    XMFLOAT4X4 planeWorld;
    XMStoreFloat4x4(&planeWorld, XMMatrixScaling(50.0f, 1.0f, 50.0f));
    m_PlaneObjectCB->Write(&planeWorld, sizeof(planeWorld));

    // Spawn 10 cubes at random positions.
    GEngine::MersenneTwister::Seed(42);
    for (uint32_t i{}; i < 10; ++i) {
        const float x{GEngine::MersenneTwister::GetRandom<float>(-10.0f, 10.0f)};
        const float y{GEngine::MersenneTwister::GetRandom<float>(0.5f, 10.0f)};
        const float z{GEngine::MersenneTwister::GetRandom<float>(-10.0f, 10.0f)};

        auto cb = renderer.CreateConstantBuffer(sizeof(XMFLOAT4X4));
        XMFLOAT4X4 worldMatrix;
        XMStoreFloat4x4(&worldMatrix, XMMatrixTranslation(x, y, z));
        cb->Write(&worldMatrix, sizeof(worldMatrix));
        m_CubeObjectCBs.push_back(std::move(cb));
    }

    m_CameraCubeObjectCB = renderer.CreateConstantBuffer(sizeof(XMFLOAT4X4));
}

void Playground::OnUpdate(float deltaTime) {
    UpdateCamera(deltaTime);
}

void Playground::OnRender() {
    auto& renderer = GetRenderer();
    renderer.DrawMesh(*m_PlaneMesh, *m_PlaneObjectCB, m_ContainerDiffuseTex->GetSRV());

    for (auto& cb : m_CubeObjectCBs)
        renderer.DrawMesh(*m_CubeMesh, *cb, m_ContainerDiffuseTex->GetSRV());

    UpdateShadowCubePosition();
    renderer.DrawMesh(*m_CubeMesh, *m_CameraCubeObjectCB, m_ContainerDiffuseTex->GetSRV(), false);
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

    m_CameraController->Update(deltaTime, input);
}

void Playground::UpdateShadowCubePosition() {
    auto& shadowCamera = GetWorld().GetShadowCamera();
    if (!shadowCamera)
        return;

    XMVECTOR shadowCameraPosition = DirectX::XMLoadFloat3(&shadowCamera->GetPosition());
    XMFLOAT4X4 worldMatrix;
    XMStoreFloat4x4(&worldMatrix, XMMatrixTranslationFromVector(shadowCameraPosition));
    m_CameraCubeObjectCB->Write(&worldMatrix, sizeof(worldMatrix));
}
