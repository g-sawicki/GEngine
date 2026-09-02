#include "common.hlsli"

struct RootConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
};

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<RootConstants> constantsCB : register(b1);
SamplerState hdrSampler : register(s0);

// https://64.github.io/tonemapping
float3 ReinhardToneMap(float3 luminance) {
    return luminance / (1.0f + luminance);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ToneMapCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    if (dispatchThreadId.x >= sceneInfoCB.screenResolution.x ||
        dispatchThreadId.y >= sceneInfoCB.screenResolution.y)
        return;

    Texture2D<float4> hdrTexture = ResourceDescriptorHeap[constantsCB.InputIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[constantsCB.OutputIndex];

    float2 uv = (float2(dispatchThreadId.xy) + 0.5f) / float2(sceneInfoCB.screenResolution);
    half4 hdrTex = hdrTexture.SampleLevel(hdrSampler, uv, 0.0f);
    float3 color = ReinhardToneMap(hdrTex.xyz);
    color = pow(color, 1.0f / 2.2f);
    outputTexture[dispatchThreadId.xy] = float4(color, 1.0f);
}