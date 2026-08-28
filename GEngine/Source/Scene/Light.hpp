#pragma once

#include <DirectXMath.h>

#include <cstdint>

namespace GEngine {

static constexpr uint32_t kMaxCascades = 4;
static constexpr uint32_t kMaxLights = 16;

enum class LightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2,
};

struct DirectionalLight {
    DirectX::XMFLOAT3 Direction{0.0f, -1.0f, 0.0f};
    float Intensity{1.0f};
    DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
};

struct PointLight {
    DirectX::XMFLOAT3 Position{};
    float Intensity{1.0f};
    DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
};

struct SpotLight {
    DirectX::XMFLOAT3 Position{};
    float Intensity{1.0f};
    DirectX::XMFLOAT3 Direction{0.0f, -1.0f, 0.0f};
    DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
    float InnerConeAngle{45.0f};
    float OuterConeAngle{60.0f};
};

struct ShadowConfig {
    bool Enabled{true};
    uint32_t MapSize{2048};
    float Bias{0.0005f};
    float SlopeScaleBias{4.0f};
    float NormalOffsetScale{1.0f};
    float MaxFarZ{200.0f};
};

struct CascadedShadowMapsData {
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

static_assert(sizeof(CascadedShadowMapsData) == 304);

struct LightData {
    DirectX::XMFLOAT3 Position{};
    uint32_t Type{};
    DirectX::XMFLOAT3 Direction{};
    DirectX::XMFLOAT3 Color{};
    float Intensity{};
    float CosInnerCone{};
    float CosOuterCone{};
};

static_assert(sizeof(LightData) == 52);

} // namespace GEngine
