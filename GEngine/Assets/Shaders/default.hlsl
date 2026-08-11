#include "light.hlsli"

struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 worldPos : WORLDPOS;
    float4 worldPosLightSpace : WORLDPOS_LIGHTSPACE;
    float3 tangent : TANGENT;
    float3 normal : NORMAL;
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

cbuffer LightDataBuffer : register(b2)
{
    LightData lightData;
};

Texture2D diffuseMap : register(t0);
Texture2D specularMap : register(t1);
Texture2D normalMap : register(t2);
Texture2D shadowMap : register(t3);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

PSInput VSMain(VSInput input)
{
    float4 worldPos = mul(input.position, world);

    PSInput output;
    output.position = mul(worldPos, viewProjection);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.worldPosLightSpace = mul(worldPos, lightData.lightViewProjection);
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), world).xyz);
    output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    return output;
}

float ComputeShadow(float4 positionLightSpace) {
    if (!lightData.shadowEnabled)
        return 1.0f;

    float3 ndc = positionLightSpace.xyz / positionLightSpace.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 1.0f - (ndc.y * 0.5f + 0.5f));

    float shadow = 0.0;
    [[unroll]]
    for (int x = -1; x <= 1; ++x) {
        [[unroll]]
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * lightData.shadowMapTexelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, uv + offset, ndc.z - lightData.shadowBias);
        }
    }

    return shadow / 9.0f;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 diffuseTex = diffuseMap.Sample(texSampler, input.uv);
    if (diffuseTex.a < 0.01f)
        discard;
    float4 specularTex = specularMap.Sample(texSampler, input.uv);
    float4 normalTex = normalMap.Sample(texSampler, input.uv);

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz - dot(input.tangent.xyz, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(normalTex.xyz * 2.0f - 1.0f, TBN));

    float3 L = normalize(directionalLight.direction);
    float3 V = normalize(cameraPosition - input.worldPos);
    float3 directionalLightColor = directionalLight.color * directionalLight.intensity;
    float NdotL = saturate(dot(worldNormal, -L));

    float shadow = ComputeShadow(input.worldPosLightSpace);

    float3 ambient = 0.3f * diffuseTex.xyz * directionalLightColor;
    float3 diffuse = diffuseTex.xyz * NdotL * directionalLightColor;

    float3 H = normalize(V - L);
    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(N, H)), 64.0f) : 0.0f;
    float3 specular = specularTex.xyz * specFactor * directionalLightColor;

    float3 color = ambient + (diffuse + specular) * shadow;
    return float4(color, diffuseTex.w);
}
