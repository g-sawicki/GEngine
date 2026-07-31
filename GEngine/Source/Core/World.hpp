#pragma once

#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"

#include <memory>

namespace GEngine {

struct SceneInfo {
    DirectX::XMFLOAT4X4 ViewProjection;
    DirectionalLight DirectionalLight;
};

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

    SceneInfo GetSceneInfo() const noexcept {
        assert(m_ActiveCamera);
        DirectX::XMFLOAT4X4 viewProjection{};
        DirectX::XMStoreFloat4x4(&viewProjection,
                                 DirectX::XMMatrixTranspose(m_ActiveCamera->GetViewProjectionMatrix()));
        return SceneInfo{
            .ViewProjection = viewProjection,
            .DirectionalLight = m_DirectionalLight,
        };
    }

  private:
    std::unique_ptr<Camera> m_ActiveCamera;
    DirectionalLight m_DirectionalLight;
};

} // namespace GEngine
