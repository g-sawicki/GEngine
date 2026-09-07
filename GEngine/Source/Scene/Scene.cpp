#include "PCH.hpp"

#include "Scene.hpp"

#include "Rendering/CascadedShadowMaps.hpp"
#include "Scene/ModelLoader.hpp"

#include <cfloat>
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
        .CameraPosition = m_Camera.GetPosition(),
        .CameraForward = m_Camera.GetForward(),
    };
    DirectX::XMStoreFloat4x4(&sceneInfo.ViewProjection, m_Camera.GetViewProjectionMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.View, m_Camera.GetViewMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.Projection, m_Camera.GetProjectionMatrix());
    return sceneInfo;
}

CascadedShadowMapsData Scene::GetCascadedShadowMapsData() const noexcept {
    CascadedShadowMapsData cascadedShadowMapsData{
        .ShadowMapTexelSize = 1.0f / static_cast<float>(m_ShadowConfig.MapSize),
        .ShadowBias = m_ShadowConfig.Bias,
        .ShadowSlopeScaleBias = m_ShadowConfig.SlopeScaleBias,
        .NormalOffsetScale = m_ShadowConfig.NormalOffsetScale,
        .ShadowEnabled = m_ShadowConfig.Enabled ? 1u : 0u,
    };

    if (m_ShadowConfig.Enabled) {
        const CSM::CascadeData cascadeData = CSM::CalculateCascadeData(m_Camera, m_DirectionalLight, m_ShadowConfig);
        cascadedShadowMapsData.CascadeCount = static_cast<uint32_t>(cascadeData.ViewProjection.size());
        for (uint32_t i{}; i < cascadedShadowMapsData.CascadeCount && i < kMaxCascades; ++i) {
            DirectX::XMStoreFloat4x4(&cascadedShadowMapsData.LightViewProjection[i], cascadeData.ViewProjection[i]);
            (&cascadedShadowMapsData.CascadeSplits.x)[i] = cascadeData.FarSplits[i];
        }
    }

    return cascadedShadowMapsData;
}

} // namespace GEngine
