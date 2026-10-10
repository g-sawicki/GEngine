#include "Interop/Common.h"
#include "Interop/Bloom.h"
#include "math.hlsli"

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<BloomConstants> constantsCB : register(b1);
SamplerState hdrSampler : register(s0);

static const float kBloomWeights[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};

float3 ExtractBrightPass(float3 color) {
    const float luminance = Luminance(color);
    const float contribution = max(luminance - 1.0f, 0.0f) / max(luminance, 1e-5f);
    return color * contribution;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void BloomCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    if (dispatchThreadId.x >= sceneInfoCB.screenResolution.x || dispatchThreadId.y >= sceneInfoCB.screenResolution.y)
        return;

    Texture2D<float4> inputTexture = ResourceDescriptorHeap[constantsCB.InputIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[constantsCB.OutputIndex];

    float2 uv = (float2(dispatchThreadId.xy) + 0.5f) / float2(sceneInfoCB.screenResolution);
    const float2 texelSize = rcp(float2(sceneInfoCB.screenResolution));
    const float2 axis = constantsCB.Horizontal != 0 ? float2(texelSize.x, 0.0f) : float2(0.0f, texelSize.y);

    float3 color = 0.0f;
    for (int offset = -4; offset <= 4; ++offset) {
        float3 sampleColor = inputTexture.SampleLevel(hdrSampler, uv + axis * offset, 0.0f).rgb;
        if (constantsCB.Horizontal != 0)
            sampleColor = ExtractBrightPass(sampleColor);

        color += sampleColor * kBloomWeights[abs(offset)];
    }

    if (constantsCB.Horizontal != 0) {
        outputTexture[dispatchThreadId.xy] = float4(color, 1.0f);
    } else {
        float4 sceneColor = outputTexture[dispatchThreadId.xy];
        outputTexture[dispatchThreadId.xy] = float4(sceneColor.rgb + color, sceneColor.a);
    }
}
