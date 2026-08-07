#include "PCH.hpp"

#include "World.hpp"

#include <cfloat>
#include <cmath>

namespace GEngine {

Camera& World::CreateCamera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position) {
    m_ActiveCamera = std::make_unique<Camera>(desc, position);
    return *m_ActiveCamera;
}

LightData World::GetLightData() const noexcept {
    assert(m_ActiveCamera);

    LightData lightData{};
    lightData.ShadowMapTexelSize = 1.0f / static_cast<float>(m_ShadowConfig.MapSize);
    lightData.ShadowBias = m_ShadowConfig.Bias;
    lightData.ShadowSlopeScaleBias = m_ShadowConfig.SlopeScaleBias;
    lightData.NormalOffsetScale = m_ShadowConfig.NormalOffsetScale;
    lightData.ShadowEnabled = m_ShadowConfig.Enabled ? 1u : 0u;

    DirectX::XMStoreFloat4x4(&lightData.LightViewProjection, ComputeLightViewProjection(*m_ActiveCamera));

    return lightData;
}

DirectX::XMMATRIX World::ComputeLightViewProjection([[maybe_unused]] const Camera& camera) const {
    DirectX::XMVECTOR lightEye = DirectX::XMVectorScale(DirectX::XMLoadFloat3(&m_DirectionalLight.Direction), -20.0f);
    DirectX::XMMATRIX lightView =
        DirectX::XMMatrixLookAtLH(lightEye, DirectX::XMVectorZero(), DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
    DirectX::XMMATRIX lightProjection = DirectX::XMMatrixOrthographicLH(21.0f, 21.0f, 0.1f, 100.0f);
    return DirectX::XMMatrixMultiply(lightView, lightProjection);
}

} // namespace GEngine
