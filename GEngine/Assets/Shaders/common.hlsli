#include "light.hlsli"

struct SceneInfo {
    row_major float4x4 viewProjection;
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 cameraPosition;
    float3 cameraForward;
    uint2 screenResolution;
    uint32_t lightCount;
    uint32_t lightIndex;
};

struct ObjectConstants {
    row_major float4x4 world;
};
