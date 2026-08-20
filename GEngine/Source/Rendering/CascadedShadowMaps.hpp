#pragma once

#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"

#include <array>
#include <vector>

namespace GEngine {

struct CascadePlane {
    std::array<DirectX::XMVECTOR, 4> Corners{};
};

class CascadedShadowMaps {
  public:
    struct CascadeData {
        std::vector<DirectX::XMMATRIX> ViewProjection;
        std::vector<float> FarSplits;
    };

    explicit CascadedShadowMaps(uint8_t cascadeCount = kMaxCascades) : m_Cascades(cascadeCount) {}

    [[nodiscard]] CascadeData Update(const Camera& camera, const DirectionalLight& directionalLight,
                                     const ShadowConfig& shadowConfig) const;

    [[nodiscard]] uint8_t GetCascadeCount() const noexcept { return m_Cascades; }

  private:
    std::vector<float> DivideFrustumIntoCascades(float nearZ, float farZ) const;
    std::vector<CascadePlane> CalculateRectangleForEachCascadeBoundary(const Camera& camera,
                                                                       const std::vector<float>& cascadeDepths) const;
    DirectX::XMVECTOR CalculateShadowCameraCenter(const CascadePlane& nearPlane, const CascadePlane& farPlane) const;

    uint8_t m_Cascades{};
};

} // namespace GEngine
