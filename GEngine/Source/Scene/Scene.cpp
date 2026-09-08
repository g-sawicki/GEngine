#include "PCH.hpp"

#include "Scene.hpp"

#include "Interop/Common.h"
#include "Interop/Light.h"
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
        .cameraPosition = m_Camera.GetPosition(),
        .cameraForward = m_Camera.GetForward(),
    };
    DirectX::XMStoreFloat4x4(&sceneInfo.viewProjection, m_Camera.GetViewProjectionMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.view, m_Camera.GetViewMatrix());
    DirectX::XMStoreFloat4x4(&sceneInfo.projection, m_Camera.GetProjectionMatrix());
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
        cascadedShadowMapsData.cascadeCount = static_cast<uint32_t>(cascadeData.ViewProjection.size());
        for (uint32_t i{}; i < cascadedShadowMapsData.cascadeCount && i < kMaxCascades; ++i) {
            DirectX::XMStoreFloat4x4(&cascadedShadowMapsData.lightViewProjection[i], cascadeData.ViewProjection[i]);
            (&cascadedShadowMapsData.cascadeSplits.x)[i] = cascadeData.FarSplits[i];
        }
    }

    return cascadedShadowMapsData;
}

} // namespace GEngine
