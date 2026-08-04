#include "light.hlsli"

struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPos : WORLDPOS;
};

cbuffer SceneInfo : register(b0)
{
    row_major float4x4 viewProjection;
    float3 cameraPosition;
    uint padding0;
    DirectionalLight directionalLight;
};

cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 world;
};

Texture2D diffuseMap : register(t0);
Texture2D specularMap : register(t1);
SamplerState texSampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(input.position, world);
    output.position = mul(worldPos, viewProjection);
    output.normal = mul(input.normal, (float3x3)world);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(directionalLight.direction);
    float3 V = normalize(cameraPosition - input.worldPos);
    float3 directionalLightColor = directionalLight.color * directionalLight.intensity;
    float NdotL = saturate(dot(N, -L));

    float4 diffuseTex = diffuseMap.Sample(texSampler, input.uv);
    float4 specularTex = specularMap.Sample(texSampler, input.uv);

    float3 ambient = 0.3f * diffuseTex.xyz * directionalLightColor;
    float3 diffuse = diffuseTex.xyz * NdotL * directionalLightColor;

    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(V, reflect(L, N))), 16) : 0.0f;
    float3 specular = specularTex.xyz * specFactor * directionalLightColor;

    float3 color = ambient + diffuse + specular;
    return float4(color, diffuseTex.w);
}
