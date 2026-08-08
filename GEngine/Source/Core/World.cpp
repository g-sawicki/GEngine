#include "PCH.hpp"

#include "World.hpp"

#include <cfloat>
#include <cmath>

namespace GEngine {

Camera& World::CreateCamera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position) {
    m_ActiveCamera = std::make_unique<Camera>(desc);
    m_ActiveCamera->SetPosition(position);
    return *m_ActiveCamera;
}

LightData World::GetLightData() const noexcept {
    if (!m_ActiveCamera || !m_ShadowCamera)
        return LightData{};

    LightData lightData{};
    lightData.ShadowMapTexelSize = 1.0f / static_cast<float>(m_ShadowConfig.MapSize);
    lightData.ShadowBias = m_ShadowConfig.Bias;
    lightData.ShadowSlopeScaleBias = m_ShadowConfig.SlopeScaleBias;
    lightData.NormalOffsetScale = m_ShadowConfig.NormalOffsetScale;
    lightData.ShadowEnabled = m_ShadowConfig.Enabled ? 1u : 0u;

    DirectX::XMStoreFloat4x4(&lightData.LightViewProjection, m_ShadowCamera->GetViewProjectionMatrix());

    return lightData;
}

void World::UpdateShadowCamera() {
    if (!m_ShadowConfig.Enabled || !m_ActiveCamera)
        return;

    constexpr float kFocusDistance = 10.0f;
    constexpr float kLightDistance = 20.0f;
    constexpr float kCoverageScale = 4.0f;

    const DirectX::XMFLOAT3 cameraForwardValue = m_ActiveCamera->GetForward();
    const DirectX::XMFLOAT3 cameraPositionValue = m_ActiveCamera->GetPosition();

    const DirectX::XMVECTOR cameraForward = DirectX::XMLoadFloat3(&cameraForwardValue);
    const DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&cameraPositionValue);

    const DirectX::XMVECTOR lightDirection = DirectX::XMLoadFloat3(&m_DirectionalLight.Direction);
    DirectX::XMFLOAT4 lightDirLengthSq;
    DirectX::XMStoreFloat4(&lightDirLengthSq, DirectX::XMVector3LengthSq(lightDirection));
    if (lightDirLengthSq.x <= 1e-6f)
        return;

    const DirectX::XMVECTOR forward = DirectX::XMVectorScale(lightDirection, 1.0f / std::sqrt(lightDirLengthSq.x));

    const float distance = m_ActiveCamera->GetNearZ() + kFocusDistance;
    const float orthoSize = distance * kCoverageScale;

    if (!m_ShadowCamera) {
        m_ShadowCamera.emplace(OrthographicDesc{
            .Width = orthoSize,
            .Height = orthoSize,
            .NearZ = m_ShadowConfig.NearZ,
            .FarZ = m_ShadowConfig.FarZ,
        });
    }

    m_ShadowCamera->SetOrthographicSize(orthoSize, orthoSize);

    const DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    const DirectX::XMVECTOR worldZ = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    DirectX::XMFLOAT4 forwardUpDot;
    DirectX::XMStoreFloat4(&forwardUpDot, DirectX::XMVector3Dot(worldUp, forward));
    const DirectX::XMVECTOR upReference = std::fabs(forwardUpDot.x) > 0.9999f ? worldZ : worldUp;

    const DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upReference, forward));
    const DirectX::XMVECTOR up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, right));

    const DirectX::XMVECTOR focusPosition =
        DirectX::XMVectorAdd(cameraPosition, DirectX::XMVectorScale(cameraForward, distance));
    const DirectX::XMVECTOR lightPosition =
        DirectX::XMVectorAdd(focusPosition, DirectX::XMVectorScale(forward, -kLightDistance));

    const float texelSize = orthoSize / static_cast<float>(m_ShadowConfig.MapSize > 0 ? m_ShadowConfig.MapSize : 1u);

    DirectX::XMFLOAT4 eyeRight, eyeUp;
    DirectX::XMStoreFloat4(&eyeRight, DirectX::XMVector3Dot(lightPosition, right));
    DirectX::XMStoreFloat4(&eyeUp, DirectX::XMVector3Dot(lightPosition, up));

    const float snappedRight = std::floor(eyeRight.x / texelSize + 0.5f) * texelSize;
    const float snappedUp = std::floor(eyeUp.x / texelSize + 0.5f) * texelSize;

    DirectX::XMVECTOR snappedPosition =
        DirectX::XMVectorMultiplyAdd(right, DirectX::XMVectorReplicate(snappedRight - eyeRight.x), lightPosition);
    snappedPosition =
        DirectX::XMVectorMultiplyAdd(up, DirectX::XMVectorReplicate(snappedUp - eyeUp.x), snappedPosition);

    const DirectX::XMVECTOR snappedFocus =
        DirectX::XMVectorAdd(focusPosition, DirectX::XMVectorSubtract(snappedPosition, lightPosition));

    DirectX::XMFLOAT3 shadowPosition{};
    DirectX::XMStoreFloat3(&shadowPosition, snappedPosition);
    DirectX::XMFLOAT3 focusPositionValue{};
    DirectX::XMStoreFloat3(&focusPositionValue, snappedFocus);
    m_ShadowCamera->SetLookAt(shadowPosition, focusPositionValue);
}

} // namespace GEngine
