#include "PCH.hpp"

#include "Camera.hpp"

#include <cmath>

namespace GEngine {

Camera::Camera(const PerspectiveDesc& desc)
    : m_ProjectionType{ProjectionType::Perspective}, m_Fov{desc.FovDegrees}, m_AspectRatio{desc.AspectRatio},
      m_NearZ{desc.NearZ}, m_FarZ{desc.FarZ} {}

Camera::Camera(const OrthographicDesc& desc)
    : m_ProjectionType{ProjectionType::Orthographic}, m_OrthoWidth{desc.Width}, m_OrthoHeight{desc.Height},
      m_NearZ{desc.NearZ}, m_FarZ{desc.FarZ} {}

void Camera::SetPosition(const DirectX::XMFLOAT3& position) {
    m_Position = position;
    m_ViewDirty = true;
}

void Camera::SetRotation(const float pitch, const float yaw) {
    m_Pitch = pitch;
    m_Yaw = yaw;
    m_ViewDirty = true;
}

void Camera::SetLookAt(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target) {
    m_Position = position;

    const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&position);
    const DirectX::XMVECTOR focus = DirectX::XMLoadFloat3(&target);
    const DirectX::XMVECTOR delta = DirectX::XMVectorSubtract(focus, eye);

    DirectX::XMFLOAT4 lengthSqValue;
    DirectX::XMStoreFloat4(&lengthSqValue, DirectX::XMVector3LengthSq(delta));

    if (lengthSqValue.x <= 1e-6f) {
        m_ViewDirty = true;
        return;
    }

    const DirectX::XMVECTOR forward = DirectX::XMVectorScale(delta, 1.0f / std::sqrt(lengthSqValue.x));

    DirectX::XMFLOAT3 forwardValue;
    DirectX::XMStoreFloat3(&forwardValue, forward);
    m_Pitch = std::asin(forwardValue.y);
    m_Yaw = std::atan2(forwardValue.x, forwardValue.z);

    m_ViewDirty = true;
}

void Camera::Translate(const float forward, const float right, const float up) {
    if (forward == 0.0f && right == 0.0f && up == 0.0f)
        return;

    const DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&m_Position);
    DirectX::XMVECTOR displacement = DirectX::XMVectorReplicate(0.0f);

    DirectX::XMVECTOR fwd, r, u;
    ComputeLocalAxes(fwd, r, u);
    if (forward != 0.0f)
        displacement = DirectX::XMVectorMultiplyAdd(fwd, DirectX::XMVectorReplicate(forward), displacement);
    if (right != 0.0f)
        displacement = DirectX::XMVectorMultiplyAdd(r, DirectX::XMVectorReplicate(right), displacement);
    if (up != 0.0f)
        displacement = DirectX::XMVectorMultiplyAdd(u, DirectX::XMVectorReplicate(up), displacement);

    DirectX::XMStoreFloat3(&m_Position, DirectX::XMVectorAdd(pos, displacement));
    m_ViewDirty = true;
}

void Camera::Rotate(const float pitchDelta, const float yawDelta) {
    m_Pitch += pitchDelta;
    m_Yaw += yawDelta;

    // Clamp pitch to avoid gimbal lock.
    constexpr float maxPitch = DirectX::XMConvertToRadians(89.0f);
    m_Pitch = std::clamp(m_Pitch, -maxPitch, maxPitch);

    // Wrap yaw to [0, 2pi).
    m_Yaw = std::fmod(m_Yaw, DirectX::XM_2PI);
    if (m_Yaw < 0.0f)
        m_Yaw += DirectX::XM_2PI;

    m_ViewDirty = true;
}

void Camera::SetAspectRatio(const float aspectRatio) {
    m_AspectRatio = aspectRatio;
    m_ProjectionDirty = true;
}

void Camera::SetFov(const float fovDegrees) {
    m_Fov = fovDegrees;
    m_ProjectionDirty = true;
}

void Camera::SetOrthographicSize(const float width, const float height) {
    m_OrthoWidth = width;
    m_OrthoHeight = height;
    m_ProjectionDirty = true;
}

void Camera::SetNearZ(const float nearZ) {
    assert(nearZ > 0.0f);
    m_NearZ = nearZ;
    m_ProjectionDirty = true;
}

void Camera::SetFarZ(const float farZ) {
    assert(farZ > m_NearZ);
    m_FarZ = farZ;
    m_ProjectionDirty = true;
}

DirectX::XMMATRIX Camera::GetViewMatrix() const {
    if (m_ViewDirty) {
        UpdateViewMatrix();
        m_ViewDirty = false;
    }
    return DirectX::XMLoadFloat4x4(&m_ViewMatrix);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const {
    if (m_ProjectionDirty) {
        UpdateProjectionMatrix();
        m_ProjectionDirty = false;
    }
    return DirectX::XMLoadFloat4x4(&m_ProjectionMatrix);
}

DirectX::XMMATRIX Camera::GetViewProjectionMatrix() const {
    return DirectX::XMMatrixMultiply(GetViewMatrix(), GetProjectionMatrix());
}

void Camera::UpdateViewMatrix() const {
    const DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&m_Position);
    DirectX::XMVECTOR forward, up;
    [[maybe_unused]] DirectX::XMVECTOR right;
    ComputeLocalAxes(forward, right, up);
    DirectX::XMStoreFloat4x4(&m_ViewMatrix, DirectX::XMMatrixLookToLH(position, forward, up));
}

void Camera::ComputeLocalAxes(DirectX::XMVECTOR& outForward, DirectX::XMVECTOR& outRight,
                              DirectX::XMVECTOR& outUp) const {
    const float sinYaw = std::sin(m_Yaw);
    const float cosYaw = std::cos(m_Yaw);
    const float sinPitch = std::sin(m_Pitch);
    const float cosPitch = std::cos(m_Pitch);

    outForward = DirectX::XMVectorSet(cosPitch * sinYaw, sinPitch, cosPitch * cosYaw, 0.0f);

    // Pick a world-up reference that is never parallel to the view direction, so the
    // right/up basis stays well-defined even for near-vertical views (e.g. a light
    // shining straight down into the shadow camera).
    const DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    const DirectX::XMVECTOR worldZ = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    DirectX::XMFLOAT4 upDot;
    DirectX::XMStoreFloat4(&upDot, DirectX::XMVector3Dot(worldUp, outForward));
    const DirectX::XMVECTOR upReference = std::fabs(upDot.x) > 0.9999f ? worldZ : worldUp;

    outRight = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upReference, outForward));
    outUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(outForward, outRight));
}

void Camera::UpdateProjectionMatrix() const {
    if (m_ProjectionType == ProjectionType::Orthographic) {
        DirectX::XMStoreFloat4x4(&m_ProjectionMatrix,
                                 DirectX::XMMatrixOrthographicLH(m_OrthoWidth, m_OrthoHeight, m_NearZ, m_FarZ));
        return;
    }

    DirectX::XMStoreFloat4x4(&m_ProjectionMatrix, DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_Fov),
                                                                                    m_AspectRatio, m_NearZ, m_FarZ));
}

DirectX::XMFLOAT3 Camera::GetForward() const noexcept {
    const float pitchCos = std::cos(m_Pitch);
    return DirectX::XMFLOAT3{pitchCos * std::sin(m_Yaw), std::sin(m_Pitch), pitchCos * std::cos(m_Yaw)};
}

DirectX::XMFLOAT3 Camera::GetRight() const noexcept {
    const float cosPitch = std::cos(m_Pitch);
    DirectX::XMVECTOR right = DirectX::XMVectorSet(cosPitch * std::cos(m_Yaw), 0.0f, -cosPitch * std::sin(m_Yaw), 0.0f);
    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(right));
    return result;
}

DirectX::XMFLOAT3 Camera::GetUp() const noexcept {
    const DirectX::XMFLOAT3 fwd = GetForward();
    const DirectX::XMFLOAT3 r = GetRight();
    const DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&fwd);
    const DirectX::XMVECTOR right = DirectX::XMLoadFloat3(&r);
    DirectX::XMFLOAT3 up;
    DirectX::XMStoreFloat3(&up, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, right)));
    return up;
}

} // namespace GEngine
