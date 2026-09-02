#pragma once

#include "Core/Utility/Defines.hpp"
#include "Rendering/CascadedShadowMaps.hpp"
#include "Rendering/Components.hpp"
#include "Scene/AssetManager.hpp"
#include "Scene/Camera.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scene/Light.hpp"
#include "Scene/Material.hpp"
#include "Scene/Skybox.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace GEngine {

struct SceneInfo {
    DirectX::XMFLOAT4X4 ViewProjection;
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 Projection;
    DirectX::XMFLOAT3 CameraPosition;
    uint32_t Padding0{};
    DirectX::XMFLOAT3 CameraForward;
    uint32_t Padding1{};
    uint32_t ScreenResolution[2]{};
    uint32_t LightCount{};
    uint32_t LightIndex{};
};
static_assert(sizeof(SceneInfo) == 240);

using ComponentRegistry = EntityRegistry<Transform, ModelComponent>;

class Scene {
  public:
    Scene() = default;

    GE_NO_COPY_NO_MOVE(Scene)

    Camera& CreateCamera(const PerspectiveDesc& desc);
    [[nodiscard]] Camera& GetActiveCamera() noexcept {
        assert(m_Camera);
        return *m_Camera;
    }
    [[nodiscard]] const Camera& GetActiveCamera() const noexcept {
        assert(m_Camera);
        return *m_Camera;
    }

    void SetSkybox(Skybox skybox) noexcept { m_Skybox = std::move(skybox); }
    const Skybox& GetSkybox() const noexcept { return m_Skybox; }

    void SetDirectionalLight(const DirectionalLight& directionalLight) noexcept {
        m_DirectionalLight = directionalLight;
    }
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept { return m_DirectionalLight; }

    void AddPointLight(const PointLight& pointLight) noexcept { m_PointLights.push_back(pointLight); }
    [[nodiscard]] const std::vector<PointLight>& GetPointLights() const noexcept { return m_PointLights; }
    void AddSpotLight(const SpotLight& spotLight) noexcept {
        assert((spotLight.Direction.x != 0.0f || spotLight.Direction.y != 0.0f || spotLight.Direction.z != 0.0f) &&
               "SpotLight direction must be non-zero.");
        assert(spotLight.InnerConeAngle >= 0.0f && spotLight.OuterConeAngle <= 90.0f &&
               spotLight.InnerConeAngle < spotLight.OuterConeAngle &&
               "SpotLight cones must satisfy 0 <= InnerConeAngle < OuterConeAngle <= 90 degrees.");
        m_SpotLights.push_back(spotLight);
    }
    [[nodiscard]] const std::vector<SpotLight>& GetSpotLights() const noexcept { return m_SpotLights; }

    void SetShadowConfig(const ShadowConfig& shadowConfig) noexcept { m_ShadowConfig = shadowConfig; }
    [[nodiscard]] const ShadowConfig& GetShadowConfig() const noexcept { return m_ShadowConfig; }
    [[nodiscard]] uint8_t GetCascadeCount() const noexcept { return m_CSM.GetCascadeCount(); }

    CascadedShadowMapsData GetCascadedShadowMapsData() const noexcept;
    SceneInfo GetSceneInfo() const noexcept;

    [[nodiscard]] ComponentRegistry& GetEntityRegistry() noexcept { return m_EntityRegistry; }
    [[nodiscard]] const ComponentRegistry& GetEntityRegistry() const noexcept { return m_EntityRegistry; }
    [[nodiscard]] AssetManager& GetAssetManager() noexcept { return m_AssetManager; }
    [[nodiscard]] const AssetManager& GetAssetManager() const noexcept { return m_AssetManager; }

  private:
    std::optional<Camera> m_Camera{};
    Skybox m_Skybox{};

    CascadedShadowMaps m_CSM{};
    ShadowConfig m_ShadowConfig{};
    DirectionalLight m_DirectionalLight{};
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;

    ComponentRegistry m_EntityRegistry{};
    AssetManager m_AssetManager{};
};

} // namespace GEngine
