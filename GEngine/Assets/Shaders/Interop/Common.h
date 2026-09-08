#ifndef SHADER_INTEROP_COMMON_H
#define SHADER_INTEROP_COMMON_H

#include "Interop.h"

struct SceneInfo {
    row_major float4x4 viewProjection;
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 cameraPosition;
    float pad0;
    float3 cameraForward;
    float pad1;
    uint2 screenResolution;
    uint lightCount;
    uint lightIndex;
};

struct ObjectData {
    row_major float4x4 world;
};

#endif // SHADER_INTEROP_COMMON_H
