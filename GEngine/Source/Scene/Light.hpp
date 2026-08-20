#pragma once

#include <DirectXMath.h>

#include <cstdint>

namespace GEngine {

static constexpr uint32_t kMaxCascades = 4;

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
    uint32_t MapSize{2048};
    float Bias{0.0005f};
    float SlopeScaleBias{4.0f};
    float NormalOffsetScale{1.0f};
    float MaxFarZ{200.0f};
};

struct LightData {
    DirectX::XMFLOAT4X4 LightViewProjection[kMaxCascades]{};
    DirectX::XMFLOAT4 CascadeSplits{};
    float ShadowMapTexelSize{};
    float ShadowBias{};
    float ShadowSlopeScaleBias{};
    float NormalOffsetScale{};
    uint32_t ShadowEnabled{};
    uint32_t CascadeCount{};
    uint32_t Padding[2]{};
};

static_assert(sizeof(LightData) == 304);

} // namespace GEngine
