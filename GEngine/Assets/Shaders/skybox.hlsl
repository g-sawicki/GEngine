#include "Interop/Common.h"

struct VSInput {
    float3 position : POSITION;
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
    float3 worldDir = input.position;

    PSInput output;
    output.worldDir = worldDir;
    output.position = mul(float4(mul(worldDir, (float3x3)sceneInfoCB.view), 1.0f), sceneInfoCB.projection);
    output.position.z = output.position.w; // lock the skybox to the far plane
    return output;
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    TextureCube skyboxTexture = ResourceDescriptorHeap[constantsCB.skyboxIndex];
    return skyboxTexture.Sample(texSampler, input.worldDir);
}
