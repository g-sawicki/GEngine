#ifndef SHADER_INTEROP_LIGHT_H
#define SHADER_INTEROP_LIGHT_H

#include "Interop.h"

static const uint32_t kMaxCascades = 4;
static const uint32_t kMaxLights = 16;

enum LightType : uint32_t {
    Directional = 0,
    Point = 1,
    Spot = 2,
};

struct CascadedShadowMapsData {
    row_major float4x4 lightViewProjection[kMaxCascades];
    float4 cascadeSplits;
    float shadowMapTexelSize;
    float shadowBias;
    float shadowSlopeScaleBias;
    float normalOffsetScale;
    uint32_t shadowEnabled;
    uint32_t cascadeCount;
    uint32_t pad0;
    uint32_t pad1;
};

struct LightData {
    float3 position;
    LightType type;
    float3 direction;
    float3 color;
    float intensity;
    float cosInnerCone;
    float cosOuterCone;
};

#endif // SHADER_INTEROP_LIGHT_H
