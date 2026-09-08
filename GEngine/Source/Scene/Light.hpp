#pragma once

#include <DirectXMath.h>

#include <cstdint>

namespace GEngine {

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
    uint8_t CascadeCount{4};
};

} // namespace GEngine
