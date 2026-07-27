#include "PCH.hpp"

#include "CameraController.hpp"

#include <cmath>

namespace GEngine {

CameraController::CameraController(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position)
    : m_Camera{desc, position} {}

void CameraController::Update(const float deltaTime, const CameraInput& input) {
    if (input.MouseDeltaX != 0.0f || input.MouseDeltaY != 0.0f) {
        m_Camera.Rotate(-input.MouseDeltaY * m_MouseSensitivity, input.MouseDeltaX * m_MouseSensitivity);
    }

    float forward{static_cast<float>(input.MoveForward) - static_cast<float>(input.MoveBack)};
    float right{static_cast<float>(input.MoveRight) - static_cast<float>(input.MoveLeft)};
    float up{static_cast<float>(input.MoveUp) - static_cast<float>(input.MoveDown)};

    const float length{std::sqrt(forward * forward + right * right + up * up)};
    if (length > 0.0f) {
        const float invLen{1.0f / length};
        forward *= invLen;
        right *= invLen;
        up *= invLen;
    }

    const float moveDelta{m_MoveSpeed * deltaTime};
    m_Camera.Translate(forward * moveDelta, right * moveDelta, up * moveDelta);
}

} // namespace GEngine
