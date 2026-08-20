#include "PCH.hpp"

#include "Scene.hpp"

#include "Scene/ModelLoader.hpp"

#include <cfloat>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace GEngine {

Camera& Scene::CreateCamera(const PerspectiveDesc& desc) {
    m_Camera.emplace(desc);
    return *m_Camera;
}

SceneInfo Scene::GetSceneInfo() const noexcept {
    SceneInfo sceneInfo{};
    if (m_Camera) {
        DirectX::XMStoreFloat4x4(&sceneInfo.ViewProjection, m_Camera->GetViewProjectionMatrix());
        sceneInfo.CameraPosition = m_Camera->GetPosition();
        sceneInfo.CameraForward = m_Camera->GetForward();
    }
    sceneInfo.DirectionalLight = m_DirectionalLight;
    return sceneInfo;
}

std::shared_ptr<const Model> Scene::AddModel(Model model) {
    auto shared = std::make_shared<const Model>(std::move(model));
    m_ModelAssets.push_back(shared);
    return shared;
}

std::shared_ptr<const Model> Scene::AddModel(const Mesh& mesh, const Material& material) {
    Model model;
    model.Meshes.push_back(mesh);
    model.Materials.push_back(material);
    return AddModel(std::move(model));
}

std::shared_ptr<const Material> Scene::AddMaterial(Material material) {
    auto shared = std::make_shared<const Material>(std::move(material));
    m_MaterialAssets.push_back(shared);
    return shared;
}

std::shared_ptr<const Model> Scene::LoadModel(const std::filesystem::path& filepath) {
    std::expected<Model, std::string> result = ModelLoader::Load(filepath);
    if (!result)
        throw std::runtime_error(result.error());
    return AddModel(std::move(*result));
}

LightData Scene::GetLightData() const noexcept {
    if (!m_Camera)
        return LightData{};

    LightData lightData{};
    lightData.ShadowMapTexelSize = 1.0f / static_cast<float>(m_ShadowConfig.MapSize);
    lightData.ShadowBias = m_ShadowConfig.Bias;
    lightData.ShadowSlopeScaleBias = m_ShadowConfig.SlopeScaleBias;
    lightData.NormalOffsetScale = m_ShadowConfig.NormalOffsetScale;
    lightData.ShadowEnabled = m_ShadowConfig.Enabled ? 1u : 0u;

    if (m_ShadowConfig.Enabled) {
        const CascadedShadowMaps::CascadeData cascadeData = m_CSM.Update(*m_Camera, m_DirectionalLight, m_ShadowConfig);
        lightData.CascadeCount = static_cast<uint32_t>(cascadeData.ViewProjection.size());
        for (uint32_t i{}; i < lightData.CascadeCount && i < kMaxCascades; ++i) {
            DirectX::XMStoreFloat4x4(&lightData.LightViewProjection[i], cascadeData.ViewProjection[i]);
            (&lightData.CascadeSplits.x)[i] = cascadeData.FarSplits[i];
        }
    }

    return lightData;
}

} // namespace GEngine
