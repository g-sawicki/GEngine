#include "Interop/Common.h"
#include "Interop/Bloom.h"
#include "math.hlsli"

ConstantBuffer<BloomConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<BloomWeights> bloomWeightsCB : register(b2);

float3 ExtractBrightPass(float3 color, float threshold) {
    const float luminance = Luminance(color);
    const float contribution = max(luminance - threshold, 0.0f) / max(luminance, 1e-5f);
    return color * contribution;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void BloomCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoords = dispatchThreadId.xy;
    if (any(pixelCoords >= sceneInfoCB.screenResolution))
        return;

    Texture2D<float3> inputTexture = ResourceDescriptorHeap[constantsCB.InputIndex];
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[constantsCB.OutputIndex];

    const int2 axis = constantsCB.Horizontal != 0 ? int2(1, 0) : int2(0, 1);
    const int2 maximum = int2(sceneInfoCB.screenResolution) - 1;

    float3 color = 0.0f;
    for (int offset = -int(constantsCB.WeightCount); offset <= int(constantsCB.WeightCount); ++offset) {
        const int2 samplePixel = clamp(int2(pixelCoords) + axis * offset, int2(0, 0), maximum);
        float3 sampleColor = inputTexture[uint2(samplePixel)];
        if (constantsCB.Horizontal != 0)
            sampleColor = ExtractBrightPass(sampleColor, constantsCB.Threshold);

        const uint weightIndex = abs(offset);
        const float weight = bloomWeightsCB.Weights[weightIndex / 4][weightIndex % 4];
        color += sampleColor * weight;
    }

    if (constantsCB.Horizontal != 0) {
        outputTexture[pixelCoords] = float4(color, 1.0f);
    } else {
        const float4 sceneColor = outputTexture[pixelCoords];
        outputTexture[pixelCoords] = float4(sceneColor.rgb + color * constantsCB.Intensity, sceneColor.a);
    }
}
