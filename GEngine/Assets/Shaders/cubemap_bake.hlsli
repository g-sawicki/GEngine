#ifndef SHADER_CUBEMAP_BAKE_H
#define SHADER_CUBEMAP_BAKE_H

#include "math.hlsli"

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
};

struct CameraData {
    row_major float4x4 viewProjection[6];
};

struct RootConstants {
    uint32_t inputIndex;
    uint32_t cameraIndex;
};

ConstantBuffer<CameraData> cameraDataCB : register(b0);
ConstantBuffer<RootConstants> constantsCB : register(b1);
SamplerState texSampler : register(s0);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(float4(input.position, 1.0), cameraDataCB.viewProjection[constantsCB.cameraIndex]);
    output.worldPosition = input.position;
    return output;
}

#endif // SHADER_CUBEMAP_BAKE_H
