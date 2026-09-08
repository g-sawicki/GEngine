#ifndef SHADER_INTEROP_LIGHT_H
#define SHADER_INTEROP_LIGHT_H

#include "Interop.h"

static const uint kMaxCascades = 4;
static const uint kMaxLights = 16;

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
    uint shadowEnabled;
    uint cascadeCount;
    uint pad0;
    uint pad1;
};

struct LightData {
    float3 position;
    uint32_t type;
    float3 direction;
    float3 color;
    float intensity;
    float cosInnerCone;
    float cosOuterCone;
};

#endif // SHADER_INTEROP_LIGHT_H
