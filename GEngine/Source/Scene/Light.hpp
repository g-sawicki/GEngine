#pragma once

#include <DirectXMath.h>

#include <cstdint>

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

struct ShadowConfig {
    bool Enabled{true};
    uint32_t MapSize{1024};
    float Bias{0.005f};
    float SlopeScaleBias{2.0f};
    float NormalOffsetScale{1.0f};
};

struct LightData {
    DirectX::XMFLOAT4X4 LightViewProjection{};
    float ShadowMapTexelSize{};
    float ShadowBias{};
    float ShadowSlopeScaleBias{};
    float NormalOffsetScale{};
    uint32_t ShadowEnabled{};
    uint32_t Padding[3]{};
};

static_assert(sizeof(LightData) == 96);

} // namespace GEngine
