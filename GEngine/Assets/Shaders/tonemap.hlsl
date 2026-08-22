#include "common.hlsli"

struct CSInput {
    uint3 GroupId : SV_GroupID;
    uint3 GroupThreadId : SV_GroupThreadID;
    uint3 DispatchThreadId : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;
};

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);

Texture2D<float4> hdr : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

SamplerState hdrSampler : register(s0);

// https://64.github.io/tonemapping
float3 ReinhardToneMap(float3 luminance) {
    return luminance / (1.0f + luminance);
}

[numthreads(8, 8, 1)]
void ToneMapCS(CSInput input) {
    float2 uv = (float2(input.DispatchThreadId.xy) + 0.5f) / float2(sceneInfoCB.screenResolution);
    half4 hdrTex = hdr.SampleLevel(hdrSampler, uv, 0.0f);
    float3 color = ReinhardToneMap(hdrTex.xyz);
    outputTexture[input.DispatchThreadId.xy] = float4(color, 1.0f);
}