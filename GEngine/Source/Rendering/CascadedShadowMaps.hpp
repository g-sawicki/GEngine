#pragma once

#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"

#include <vector>

namespace GEngine::CSM {

struct CascadeData {
    std::vector<DirectX::XMMATRIX> ViewProjection;
    std::vector<float> FarSplits;
};

[[nodiscard]] CascadeData CalculateCascadeData(const Camera& camera, const DirectionalLight& directionalLight,
                                               const ShadowConfig& shadowConfig);

} // namespace GEngine::CSM
