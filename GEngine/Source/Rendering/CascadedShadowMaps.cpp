#include "PCH.hpp"

#include "CascadedShadowMaps.hpp"

#include <cassert>
#include <cmath>

namespace GEngine {

CascadedShadowMaps::CascadeData CascadedShadowMaps::Update(const Camera& camera,
                                                           const DirectionalLight& directionalLight,
                                                           const ShadowConfig& shadowConfig) const {
    float farZ = std::min(camera.GetFarZ(), shadowConfig.MaxFarZ);
    std::vector<float> cascadeDepths = DivideFrustumIntoCascades(camera.GetNearZ(), farZ);
    std::vector<CascadePlane> planes = CalculateRectangleForEachCascadeBoundary(camera, cascadeDepths);

    CascadeData result{};
    result.ViewProjection.reserve(m_Cascades);
    result.FarSplits.reserve(m_Cascades);

    for (uint8_t i{}; i < planes.size() - 1; ++i) {
        DirectX::XMVECTOR center = CalculateShadowCameraCenter(planes[i], planes[i + 1]);

        float radius = 0.0f;
        for (const auto& plane : {planes[i], planes[i + 1]}) {
            for (const auto& corner : plane.Corners) {
                const float dist =
                    DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(corner, center)));
                radius = std::max(radius, dist);
            }
        }
        float casterMargin = 100.0f;
        float totalOffset = radius + casterMargin;

        const DirectX::XMVECTOR lightDirection =
            DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&directionalLight.Direction));
        DirectX::XMVECTOR lightPosition =
            DirectX::XMVectorSubtract(center, DirectX::XMVectorScale(lightDirection, totalOffset));

        const DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        const DirectX::XMVECTOR worldZ = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        const DirectX::XMVECTOR upVector =
            std::fabs(DirectX::XMVectorGetX(DirectX::XMVector3Dot(worldUp, lightDirection))) > 0.9999f ? worldZ
                                                                                                       : worldUp;
        const DirectX::XMVECTOR lightRight =
            DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upVector, lightDirection));
        const DirectX::XMVECTOR lightUp =
            DirectX::XMVector3Normalize(DirectX::XMVector3Cross(lightDirection, lightRight));

        const float texelSize = 2.0f * radius / static_cast<float>(shadowConfig.MapSize);

        DirectX::XMFLOAT4 eyeRight, eyeUp;
        DirectX::XMStoreFloat4(&eyeRight, DirectX::XMVector3Dot(lightPosition, lightRight));
        DirectX::XMStoreFloat4(&eyeUp, DirectX::XMVector3Dot(lightPosition, lightUp));

        const float snappedRight = std::floor(eyeRight.x / texelSize + 0.5f) * texelSize;
        const float snappedUp = std::floor(eyeUp.x / texelSize + 0.5f) * texelSize;

        DirectX::XMVECTOR snappedPosition = DirectX::XMVectorMultiplyAdd(
            lightRight, DirectX::XMVectorReplicate(snappedRight - eyeRight.x), lightPosition);
        snappedPosition =
            DirectX::XMVectorMultiplyAdd(lightUp, DirectX::XMVectorReplicate(snappedUp - eyeUp.x), snappedPosition);

        const DirectX::XMVECTOR snappedFocus =
            DirectX::XMVectorAdd(center, DirectX::XMVectorSubtract(snappedPosition, lightPosition));

        DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(snappedPosition, snappedFocus, upVector);
        DirectX::XMMATRIX lightProj =
            DirectX::XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius, 0.0f, totalOffset + radius);

        result.ViewProjection.push_back(DirectX::XMMatrixMultiply(lightView, lightProj));
        result.FarSplits.push_back(cascadeDepths[i + 1]);
    }

    return result;
}

std::vector<float> CascadedShadowMaps::DivideFrustumIntoCascades(float nearZ, float farZ) const {
    std::vector<float> result;
    result.reserve(m_Cascades);

    constexpr float kPracticalSplitLambda = 0.85f;
    for (uint8_t i = 0; i <= m_Cascades; ++i) {
        const float fraction = static_cast<float>(i) / static_cast<float>(m_Cascades);
        const float linearDepth = nearZ + (farZ - nearZ) * fraction;
        const float logarithmicDepth = nearZ * std::pow(farZ / nearZ, fraction);
        const float depth = linearDepth * (1.0f - kPracticalSplitLambda) + logarithmicDepth * kPracticalSplitLambda;
        result.push_back(depth);
    }
    return result;
}

std::vector<CascadePlane>
CascadedShadowMaps::CalculateRectangleForEachCascadeBoundary(const Camera& camera,
                                                             const std::vector<float>& cascadeDepths) const {
    const DirectX::XMMATRIX inverseViewMatrix = DirectX::XMMatrixInverse(nullptr, camera.GetViewMatrix());
    const float fovRadians = DirectX::XMConvertToRadians(camera.GetFov());
    const float aspectRatio = camera.GetAspectRatio();

    std::vector<CascadePlane> worldSpaceCorners;
    worldSpaceCorners.reserve(cascadeDepths.size());

    for (const float depth : cascadeDepths) {
        auto halfHeight = std::tan(fovRadians / 2.0f) * depth;
        auto halfWidth = halfHeight * aspectRatio;

        DirectX::XMVECTOR bottomLeftViewSpace = DirectX::XMVectorSet(-halfWidth, -halfHeight, depth, 1.0f);
        DirectX::XMVECTOR bottomRightViewSpace = DirectX::XMVectorSet(halfWidth, -halfHeight, depth, 1.0f);
        DirectX::XMVECTOR topLeftViewSpace = DirectX::XMVectorSet(-halfWidth, halfHeight, depth, 1.0f);
        DirectX::XMVECTOR topRightViewSpace = DirectX::XMVectorSet(halfWidth, halfHeight, depth, 1.0f);

        worldSpaceCorners.emplace_back(std::array<DirectX::XMVECTOR, 4>{
            DirectX::XMVector3Transform(bottomLeftViewSpace, inverseViewMatrix),
            DirectX::XMVector3Transform(bottomRightViewSpace, inverseViewMatrix),
            DirectX::XMVector3Transform(topLeftViewSpace, inverseViewMatrix),
            DirectX::XMVector3Transform(topRightViewSpace, inverseViewMatrix),
        });
    }
    return worldSpaceCorners;
}

DirectX::XMVECTOR CascadedShadowMaps::CalculateShadowCameraCenter(const CascadePlane& nearPlane,
                                                                  const CascadePlane& farPlane) const {
    DirectX::XMVECTOR totalSum = DirectX::XMVectorZero();
    for (const auto& corner : nearPlane.Corners)
        totalSum = DirectX::XMVectorAdd(totalSum, corner);
    for (const auto& corner : farPlane.Corners)
        totalSum = DirectX::XMVectorAdd(totalSum, corner);

    DirectX::XMVECTOR center = DirectX::XMVectorScale(totalSum, 0.125f); // Average of 8 corners
    return center;
}

} // namespace GEngine
