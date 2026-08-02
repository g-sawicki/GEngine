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
    row_major float4x4 viewProjection;
    DirectionalLight directionalLight;
};

cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 world;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(input.position, world);
    output.position = mul(worldPos, viewProjection);
    output.normal = mul(input.normal, (float3x3)world);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-directionalLight.direction);
    float intensity = saturate(dot(N, L)) * directionalLight.intensity;
    float3 color = input.color.xyz * (directionalLight.color * intensity);
    return float4(color, input.color.w);
}
