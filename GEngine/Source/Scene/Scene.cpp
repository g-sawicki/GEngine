#include "PCH.hpp"

#include "Scene.hpp"

#include "Core/Log.hpp"
#include "Interop/Common.h"
#include "Interop/Light.h"
#include "Rendering/CascadedShadowMaps.hpp"
#include "Scene/ModelLoader.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace GEngine {

Camera& Scene::CreateCamera(const PerspectiveDesc& desc) {
    m_Camera = Camera(desc);
    return m_Camera;
}

SceneInfo Scene::GetSceneInfo() const noexcept {
    SceneInfo sceneInfo{
        .cameraPosition = m_Camera.GetPosition(),
        .cameraForward = m_Camera.GetForward(),
    };
    DirectX::XMStoreFloat4x4(&sceneInfo.viewProjection, m_Camera.GetViewProjectionMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.view, m_Camera.GetViewMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.projection, m_Camera.GetProjectionMatrix());
    DirectX::XMStoreFloat4x4(
        &sceneInfo.inverseViewProjection,
        DirectX::XMMatrixInverse(nullptr,
                                 m_Camera.GetViewProjectionMatrix())); // TODO: call GetViewProjectionMatrix once
    DirectX::XMStoreFloat4x4(&sceneInfo.inverseProjection,
                             DirectX::XMMatrixInverse(nullptr, m_Camera.GetProjectionMatrix()));
    return sceneInfo;
}

CascadedShadowMapsData Scene::GetCascadedShadowMapsData() const noexcept {
    CascadedShadowMapsData cascadedShadowMapsData{
        .shadowMapTexelSize = 1.0f / static_cast<float>(m_ShadowConfig.MapSize),
        .shadowBias = m_ShadowConfig.Bias,
        .shadowSlopeScaleBias = m_ShadowConfig.SlopeScaleBias,
        .normalOffsetScale = m_ShadowConfig.NormalOffsetScale,
        .shadowEnabled = m_ShadowConfig.Enabled ? 1u : 0u,
    };

    if (m_ShadowConfig.Enabled) {
        const CSM::CascadeData cascadeData = CSM::CalculateCascadeData(m_Camera, m_DirectionalLight, m_ShadowConfig);
        cascadedShadowMapsData.cascadeCount =
            std::min<uint32_t>(static_cast<uint32_t>(cascadeData.ViewProjection.size()), kMaxCascades);
        for (uint32_t i{}; i < cascadedShadowMapsData.cascadeCount; ++i) {
            DirectX::XMStoreFloat4x4(&cascadedShadowMapsData.lightViewProjection[i], cascadeData.ViewProjection[i]);
            (&cascadedShadowMapsData.cascadeSplits.x)[i] = cascadeData.FarSplits[i];
        }
    }

    return cascadedShadowMapsData;
}

std::vector<LightData> Scene::GetLightData() const {
    const auto totalLightCount = static_cast<uint32_t>(1 + m_PointLights.size() + m_SpotLights.size());
    if (totalLightCount > kMaxLights) {
        GE_CORE_WARN("Scene has exceeded the maximum number of lights {}/{}.", totalLightCount, kMaxLights);
    }

    std::vector<LightData> lightData;
    lightData.reserve(std::min(totalLightCount, kMaxLights));
    lightData.push_back({
        .position = {},
        .type = LightType::Directional,
        .direction = m_DirectionalLight.Direction,
        .color = m_DirectionalLight.Color,
        .intensity = m_DirectionalLight.Intensity,
    });
    for (const PointLight& pointLight : m_PointLights) {
        if (lightData.size() >= kMaxLights)
            break;
        lightData.push_back({
            .position = pointLight.Position,
            .type = LightType::Point,
            .color = pointLight.Color,
            .intensity = pointLight.Intensity,
        });
    }
    for (const SpotLight& spotLight : m_SpotLights) {
        if (lightData.size() >= kMaxLights)
            break;
        lightData.push_back({
            .position = spotLight.Position,
            .type = LightType::Spot,
            .direction = spotLight.Direction,
            .color = spotLight.Color,
            .intensity = spotLight.Intensity,
            .cosInnerCone = std::cos(DirectX::XMConvertToRadians(spotLight.InnerConeAngle)),
            .cosOuterCone = std::cos(DirectX::XMConvertToRadians(spotLight.OuterConeAngle)),
        });
    }

    return lightData;
}

} // namespace GEngine
