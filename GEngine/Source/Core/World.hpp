#pragma once

#include "Rendering/Mesh.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Entity.hpp"
#include "Scene/EntityManager.hpp"
#include "Scene/Light.hpp"
#include "Scene/Material.hpp"
#include "Scene/Model.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace GEngine {

struct SceneInfo {
    DirectX::XMFLOAT4X4 ViewProjection;
    DirectX::XMFLOAT3 CameraPosition;
    uint32_t Padding0{};
    DirectionalLight DirectionalLight;
};
static_assert(sizeof(SceneInfo) == 108);

class World {
  public:
    World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    Camera& CreateCamera(const PerspectiveDesc& desc);
    [[nodiscard]] Camera& GetActiveCamera() noexcept {
        assert(m_Camera);
        return *m_Camera;
    }
    [[nodiscard]] const Camera& GetActiveCamera() const noexcept {
        assert(m_Camera);
        return *m_Camera;
    }

    void SetDirectionalLight(const DirectionalLight& directionalLight) noexcept {
        m_DirectionalLight = directionalLight;
    }
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept { return m_DirectionalLight; }

    void SetShadowConfig(const ShadowConfig& shadowConfig) noexcept { m_ShadowConfig = shadowConfig; }
    [[nodiscard]] const ShadowConfig& GetShadowConfig() const noexcept { return m_ShadowConfig; }
    [[nodiscard]] const std::optional<Camera>& GetShadowCamera() const noexcept { return m_ShadowCamera; }

    void UpdateShadowCamera();
    LightData GetLightData() const noexcept;
    SceneInfo GetSceneInfo() const noexcept;

    std::shared_ptr<const Model> AddModel(Model model);
    std::shared_ptr<const Model> AddModel(const Mesh& mesh, const Material& material);
    std::shared_ptr<const Material> AddMaterial(Material material);
    std::shared_ptr<const Model> LoadModel(const std::filesystem::path& filepath);

    [[nodiscard]] EntityManager& GetEntityManager() noexcept { return m_EntityManager; }
    [[nodiscard]] const EntityManager& GetEntityManager() const noexcept { return m_EntityManager; }

  private:
    std::optional<Camera> m_Camera{};
    std::optional<Camera> m_ShadowCamera{};
    DirectionalLight m_DirectionalLight{};
    ShadowConfig m_ShadowConfig{};

    std::vector<std::shared_ptr<const Model>> m_ModelAssets;
    std::vector<std::shared_ptr<const Material>> m_MaterialAssets;

    EntityManager m_EntityManager;
};

} // namespace GEngine
