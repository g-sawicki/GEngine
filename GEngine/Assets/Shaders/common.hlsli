#include "light.hlsli"

struct SceneInfo {
    row_major float4x4 viewProjection;
    float3 cameraPosition;
    float3 cameraForward;
    DirectionalLight directionalLight;
    uint2 screenResolution;
};

struct ObjectConstants {
    row_major float4x4 world;
};
