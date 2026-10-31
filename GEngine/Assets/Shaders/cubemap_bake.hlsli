#ifndef SHADER_CUBEMAP_BAKE_H
#define SHADER_CUBEMAP_BAKE_H

#include "Interop/CubeMapBake.h"
#include "math.hlsli"

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 cubeDirection : TEXCOORD0;
};

ConstantBuffer<CubeMapBakeConstants> constantsCB : register(b0);
ConstantBuffer<CubeFaceCameraData> cameraDataCB : register(b1);
SamplerState texSampler : register(s0);
SamplerState equirectSampler : register(s1);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    PSInput output;
    const row_major float4x4 cameraViewProjection = cameraDataCB.ViewProjection[constantsCB.CameraIndex];
    output.position = mul(float4(input.position, 1.0), cameraViewProjection);
    output.cubeDirection = input.position;
    return output;
}

#endif // SHADER_CUBEMAP_BAKE_H
