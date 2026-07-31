#pragma once

#include <DirectXMath.h>

namespace GEngine {

struct DirectionalLight {
    DirectX::XMFLOAT3 Direction{};
    float Intensity{1.0f};
    DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
};

static_assert(sizeof(DirectionalLight) == 28);

struct PointLight {
    DirectX::XMFLOAT3 Position{};
    float Intensity{1.0f};
    DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
    float AttenuationConstant{};
    float AttenuationLinear{};
    float AttenuationQuadratic{};
};

static_assert(sizeof(PointLight) == 40);

} // namespace GEngine
