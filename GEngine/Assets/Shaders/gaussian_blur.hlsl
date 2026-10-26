#include "Interop/Common.h"
#include "Interop/GaussianBlur.h"

ConstantBuffer<GaussianBlurConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<GaussianWeights> gaussianWeightsCB : register(b2);

[shader("compute")]
[numthreads(8, 8, 1)]
void GaussianBlurCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    if (dispatchThreadId.x >= sceneInfoCB.screenResolution.x || dispatchThreadId.y >= sceneInfoCB.screenResolution.y)
        return;

    Texture2D<float> inputTexture = ResourceDescriptorHeap[constantsCB.InputIndex];
    RWTexture2D<float> outputTexture = ResourceDescriptorHeap[constantsCB.OutputIndex];

    const int2 pixel = int2(dispatchThreadId.xy);
    const int2 axis = constantsCB.Horizontal != 0 ? int2(1, 0) : int2(0, 1);
    const int2 maximum = int2(sceneInfoCB.screenResolution) - 1;

    float occlusion = 0.0f;
    for (int offset = -int(constantsCB.WeightCount); offset <= int(constantsCB.WeightCount); ++offset) {
        const int2 samplePixel = clamp(pixel + axis * offset, int2(0, 0), maximum);
        const uint weightIndex = abs(offset);
        const float weight = gaussianWeightsCB.Weights[weightIndex / 4][weightIndex % 4];
        occlusion += inputTexture.Load(int3(samplePixel, 0)) * weight;
    }

    outputTexture[pixel] = occlusion;
}
