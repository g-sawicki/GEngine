#pragma once

#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"

#include <memory>

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

    // Camera
    Camera& CreateCamera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position = {});
    [[nodiscard]] Camera* GetActiveCamera() const noexcept { return m_ActiveCamera.get(); }

    void SetDirectionalLight(const DirectionalLight& directionalLight) noexcept {
        m_DirectionalLight = directionalLight;
    }
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept { return m_DirectionalLight; }

    void SetShadowConfig(const ShadowConfig& shadowConfig) noexcept { m_ShadowConfig = shadowConfig; }
    [[nodiscard]] const ShadowConfig& GetShadowConfig() const noexcept { return m_ShadowConfig; }

    SceneInfo GetSceneInfo() const noexcept {
        assert(m_ActiveCamera);
        DirectX::XMFLOAT4X4 viewProjection{};
        DirectX::XMStoreFloat4x4(&viewProjection, m_ActiveCamera->GetViewProjectionMatrix());
        return SceneInfo{
            .ViewProjection = viewProjection,
            .CameraPosition = m_ActiveCamera->GetPosition(),
            .DirectionalLight = m_DirectionalLight,
        };
    }

    LightData GetLightData() const noexcept;

  private:
    DirectX::XMMATRIX ComputeLightViewProjection(const Camera& camera) const;

    std::unique_ptr<Camera> m_ActiveCamera;
    DirectionalLight m_DirectionalLight;
    ShadowConfig m_ShadowConfig;
};

} // namespace GEngine
