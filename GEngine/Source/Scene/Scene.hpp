#pragma once

#include "Core/Utility/Defines.hpp"
#include "Rendering/Components.hpp"
#include "Scene/AssetManager.hpp"
#include "Scene/Camera.hpp"
#include "Scene/EntityRegistry.hpp"
#include "Scene/Light.hpp"
#include "Scene/Skybox.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

struct SceneInfo;
struct CascadedShadowMapsData;

namespace GEngine {

using ComponentRegistry = EntityRegistry<Transform, ModelComponent>;

class Scene {
  public:
    Scene() = default;

    GE_NO_COPY_NO_MOVE(Scene)

    Camera& CreateCamera(const PerspectiveDesc& desc);
    [[nodiscard]] Camera& GetActiveCamera() noexcept { return m_Camera; }
    [[nodiscard]] const Camera& GetActiveCamera() const noexcept { return m_Camera; }

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

    CascadedShadowMapsData GetCascadedShadowMapsData() const noexcept;
    SceneInfo GetSceneInfo() const noexcept;

    [[nodiscard]] ComponentRegistry& GetEntityRegistry() noexcept { return m_EntityRegistry; }
    [[nodiscard]] const ComponentRegistry& GetEntityRegistry() const noexcept { return m_EntityRegistry; }

  private:
    Camera m_Camera{};
    Skybox m_Skybox{};

    ShadowConfig m_ShadowConfig{};
    DirectionalLight m_DirectionalLight{};
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;

    ComponentRegistry m_EntityRegistry{};
};

} // namespace GEngine
