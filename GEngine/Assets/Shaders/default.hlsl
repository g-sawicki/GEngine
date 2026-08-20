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
    float3 tangent : TANGENT;
    float3 normal : NORMAL;
};

cbuffer SceneInfo : register(b0)
{
    row_major float4x4 viewProjection;
    float3 cameraPosition;
    float3 cameraForward;
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
Texture2DArray shadowMap : register(t3);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

PSInput VSMain(VSInput input)
{
    float4 worldPos = mul(input.position, world);

    PSInput output;
    output.position = mul(worldPos, viewProjection);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), world).xyz);
    output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    return output;
}

uint SelectCascade(float viewDepth)
{
    if (viewDepth <= lightData.cascadeSplits.x)
        return 0;
    if (viewDepth <= lightData.cascadeSplits.y)
        return 1;
    if (viewDepth <= lightData.cascadeSplits.z)
        return 2;
    return 3;
}

float SampleCascadeShadow(float3 worldPos, uint cascade)
{
    float4 positionLightSpace = mul(float4(worldPos, 1.0f), lightData.lightViewProjection[cascade]);
    float3 ndc = positionLightSpace.xyz / positionLightSpace.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 1.0f - (ndc.y * 0.5f + 0.5f));

    float slope = max(abs(ddx(ndc.z)), abs(ddy(ndc.z)));
    float bias = min(lightData.shadowSlopeScaleBias * slope, 0.001f) + lightData.shadowBias;

    float shadow = 0.0;
    [[unroll]]
    for (int x = -1; x <= 1; ++x) {
        [[unroll]]
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * lightData.shadowMapTexelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, float3(uv + offset, cascade), ndc.z - bias);
        }
    }

    return shadow / 9.0f;
}

float ComputeShadow(float3 worldPos)
{
    if (!lightData.shadowEnabled || lightData.cascadeCount == 0)
        return 1.0f;

    float viewDepth = dot(worldPos - cameraPosition, cameraForward);
    uint cascade = SelectCascade(viewDepth);

    float split = cascade == 0 ? lightData.cascadeSplits.x :
                  cascade == 1 ? lightData.cascadeSplits.y :
                  cascade == 2 ? lightData.cascadeSplits.z : 1e30f;
    float blendStart = split * 0.8f;
    float blend = saturate((viewDepth - blendStart) / max(split - blendStart, 1e-6f));

    float shadow = SampleCascadeShadow(worldPos, cascade);
    if (cascade < lightData.cascadeCount - 1)
        shadow = lerp(shadow, SampleCascadeShadow(worldPos, cascade + 1), blend);

    return shadow;
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

    float shadow = ComputeShadow(input.worldPos);

    float3 ambient = 0.3f * diffuseTex.xyz * directionalLightColor;
    float3 diffuse = diffuseTex.xyz * NdotL * directionalLightColor;

    float3 H = normalize(V - L);
    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(N, H)), 64.0f) : 0.0f;
    float3 specular = specularTex.xyz * specFactor * directionalLightColor;

    float3 color = ambient + (diffuse + specular) * shadow;
    return float4(color, diffuseTex.w);
}
