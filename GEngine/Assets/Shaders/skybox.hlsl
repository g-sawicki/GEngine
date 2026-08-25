#include "common.hlsli"

struct VSInput {
    float4 position : POSITION;
};

struct PSInput {
    float3 worldDir : WORLDDIR;
    float4 position : SV_POSITION;
};

struct RootConstants {
    uint skyboxIndex;
};

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<RootConstants> constantsCB : register(b1);
SamplerState texSampler : register(s0);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    float3 worldDir = input.position.xyz;

    PSInput output;
    output.worldDir = worldDir;
    output.position = mul(float4(mul(worldDir, (float3x3)sceneInfoCB.view), 1.0f), sceneInfoCB.projection);
    output.position.z = output.position.w; // lock the skybox to the far plane
    return output;
}

// Map a world-space direction to a UV in an equirectangular (2:1) panorama.
float2 DirectionToEquirectUV(float3 direction) {
    const float kPi = 3.14159265358979323846;
    float3 d = normalize(direction);
    // horizontal angle in [-pi, pi]
    float theta = atan2(d.z, d.x);
    // vertical angle in [-pi/2, pi/2]
    float phi = asin(clamp(d.y, -1.0f, 1.0f));
    return float2(0.5f + theta / (2.0f * kPi), 0.5f - phi / kPi);
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    Texture2D skyboxTexture = ResourceDescriptorHeap[constantsCB.skyboxIndex];
    return skyboxTexture.Sample(texSampler, DirectionToEquirectUV(input.worldDir));
}
