#include "light.hlsli"

struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

cbuffer SceneInfo : register(b0)
{
    float4x4 viewProjection;
    DirectionalLight directionalLight;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(input.position, viewProjection);
    output.normal = input.normal;
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float intensity = dot(input.normal, -directionalLight.direction) * directionalLight.intensity;
    float3 color = input.color.xyz * (directionalLight.color * intensity);
    return float4(color, input.color.w);
}
