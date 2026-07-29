#pragma once

#include "Scene/Camera.hpp"

#include <memory>

namespace GEngine {

class World {
  public:
    World() = default;

    // Camera
    Camera& CreateCamera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position = {});
    [[nodiscard]] Camera* GetActiveCamera() const noexcept { return m_ActiveCamera.get(); }

  private:
    std::unique_ptr<Camera> m_ActiveCamera;
};

} // namespace GEngine
