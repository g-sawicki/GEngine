#include "light.hlsli"

struct VSInput
{
    float4 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

cbuffer LightDataBuffer : register(b0)
{
    LightData lightData;
};

cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 world;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(input.position, world);
    output.position = mul(worldPos, lightData.lightViewProjection);
    return output;
}

void PSMain(PSInput input){}
