#include "Playground.hpp"

#include "Scene/Light.hpp"

#include <cmath>

void Playground::OnInit() {
    auto& world = GetWorld();

    auto& camera = world.CreateCamera(
        {.FovDegrees = 70.0f, .AspectRatio = 1280.0f / 720.0f, .NearZ = 0.1f, .FarZ = 1000.0f}, {0.0f, 0.0f, -5.0f});
    m_CameraController = std::make_unique<GEngine::CameraController>(camera);

    GEngine::DirectionalLight directionalLight{
        .Intensity = 1.0f,
        .Color = {1.0f, 1.0f, 1.0f},
    };
    DirectX::XMStoreFloat3(&directionalLight.Direction,
                           DirectX::XMVector3Normalize(DirectX::XMVectorSet(2.0f, -1.0f, 4.0f, 0.0f)));
    world.SetDirectionalLight(directionalLight);
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

    m_CameraController->Update(deltaTime, input);
}
