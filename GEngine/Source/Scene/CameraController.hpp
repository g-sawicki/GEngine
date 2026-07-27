#pragma once

#include "Camera.hpp"

namespace GEngine {

struct CameraInput {
    bool MoveForward{};
    bool MoveBack{};
    bool MoveRight{};
    bool MoveLeft{};
    bool MoveUp{};
    bool MoveDown{};
    float MouseDeltaX{};
    float MouseDeltaY{};
};

class CameraController {
  public:
    CameraController(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position = {});

    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;
    CameraController(CameraController&&) = delete;
    CameraController& operator=(CameraController&&) = delete;

    /// Call each frame with the accumulated input for this frame.
    void Update(float deltaTime, const CameraInput& input);

    [[nodiscard]] const Camera& GetCamera() const noexcept { return m_Camera; }
    [[nodiscard]] Camera& GetCamera() noexcept { return m_Camera; }

    [[nodiscard]] float GetMoveSpeed() const noexcept { return m_MoveSpeed; }
    [[nodiscard]] float GetMouseSensitivity() const noexcept { return m_MouseSensitivity; }
    void SetMoveSpeed(float speed) noexcept { m_MoveSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) noexcept { m_MouseSensitivity = sensitivity; }

  private:
    Camera m_Camera;
    float m_MoveSpeed{5.0f};
    float m_MouseSensitivity{0.002f};
};

} // namespace GEngine
