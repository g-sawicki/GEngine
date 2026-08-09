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
    CameraController() = default;
    explicit CameraController(Camera* camera) noexcept : m_Camera(camera) {}

    /// Call each frame with the accumulated input for this frame.
    void Update(float deltaTime, const CameraInput& input);

    [[nodiscard]] float GetMoveSpeed() const noexcept { return m_MoveSpeed; }
    [[nodiscard]] float GetMouseSensitivity() const noexcept { return m_MouseSensitivity; }
    void SetMoveSpeed(float speed) noexcept { m_MoveSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) noexcept { m_MouseSensitivity = sensitivity; }

  private:
    Camera* m_Camera{};
    float m_MoveSpeed{5.0f};
    float m_MouseSensitivity{0.002f};
};

} // namespace GEngine
