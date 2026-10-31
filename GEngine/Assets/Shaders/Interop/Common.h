#ifndef SHADER_INTEROP_COMMON_H
#define SHADER_INTEROP_COMMON_H

#include "Interop.h"

static const uint32_t kInvalidBindlessIndex = 0xFFFFFFFF;

struct SceneInfo {
    row_major float4x4 viewProjection;
    row_major float4x4 view;
    row_major float4x4 projection;
    row_major float4x4 inverseViewProjection;
    row_major float4x4 inverseProjection;
    float3 cameraPosition;
    float pad0;
    float3 cameraForward;
    float pad1;
    uint2 screenResolution;
    uint32_t lightCount;
    uint32_t lightIndex;
    uint32_t irradianceIndex;
    uint32_t pad2;
    uint32_t pad3;
    uint32_t pad4;
};

struct ObjectData {
    row_major float4x4 world;
    row_major float4x4 normal;
};

#endif // SHADER_INTEROP_COMMON_H
