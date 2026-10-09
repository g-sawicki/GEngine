#include "Interop/Common.h"
#include "Interop/ToneMap.h"

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<ToneMapRootConstants> constantsCB : register(b1);
SamplerState hdrSampler : register(s0);

// https://64.github.io/tonemapping
float Luminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 ReinhardToneMap(float3 color) {
    const float luminance = Luminance(color);
    return color / (1.0f + luminance);
}

float3 ExtendedReinhardToneMap(float3 color, float maxWhite) {
    const float luminance = Luminance(color);
    const float whitePoint = max(maxWhite, 1e-6f);
    return color * (1.0f + luminance / (whitePoint * whitePoint)) / (1.0f + luminance);
}

float3 Uncharted2ToneMap(float3 color) {
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

float3 Uncharted2FilmicToneMap(float3 color) {
    const float exposureBias = 2.0f;
    const float3 partial = Uncharted2ToneMap(color * exposureBias);
    const float3 whiteScale = 1.0f / Uncharted2ToneMap(11.2f);
    return partial * whiteScale;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ToneMapCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    if (dispatchThreadId.x >= sceneInfoCB.screenResolution.x || dispatchThreadId.y >= sceneInfoCB.screenResolution.y)
        return;

    Texture2D<float4> hdrTexture = ResourceDescriptorHeap[constantsCB.InputIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[constantsCB.OutputIndex];

    float2 uv = (float2(dispatchThreadId.xy) + 0.5f) / float2(sceneInfoCB.screenResolution);
    half4 hdrTex = hdrTexture.SampleLevel(hdrSampler, uv, 0.0f);

    const float3 hdrColor = max(hdrTex.xyz, 0.0f);

    float3 color;
    switch (constantsCB.TonemapMode) {
    case ToneMapMode::Uncharted2:
        color = Uncharted2FilmicToneMap(hdrColor);
        break;
    case ToneMapMode::ExtendedReinhard:
        color = ExtendedReinhardToneMap(hdrColor, constantsCB.MaxWhite);
        break;
    case ToneMapMode::Reinhard:
    default:
        color = ReinhardToneMap(hdrColor);
        break;
    }

    color = pow(color, 1.0f / 2.2f);
    outputTexture[dispatchThreadId.xy] = float4(color, 1.0f);
}
