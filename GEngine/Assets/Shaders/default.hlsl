#include "common.hlsli"

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

struct RootConstants{
    uint32_t diffuseIndex;
    uint32_t specularIndex;
    uint32_t normalIndex;
    uint32_t shadowIndex;
};

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<LightData> lightDataCB : register(b1);
ConstantBuffer<ObjectConstants> objectConstantsCB : register(b2);
ConstantBuffer<RootConstants> constantsCB : register(b3);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

PSInput VSMain(VSInput input)
{
    float4 worldPos = mul(input.position, objectConstantsCB.world);

    PSInput output;
    output.position = mul(worldPos, sceneInfoCB.viewProjection);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), objectConstantsCB.world).xyz);
    output.normal = normalize(mul(float4(input.normal, 0.0f), objectConstantsCB.world).xyz);
    return output;
}

uint SelectCascade(float viewDepth)
{
    if (viewDepth <= lightDataCB.cascadeSplits.x)
        return 0;
    if (viewDepth <= lightDataCB.cascadeSplits.y)
        return 1;
    if (viewDepth <= lightDataCB.cascadeSplits.z)
        return 2;
    return 3;
}

float SampleCascadeShadow(float3 worldPos, Texture2DArray shadowMap, uint cascade)
{
    float4 positionLightSpace = mul(float4(worldPos, 1.0f), lightDataCB.lightViewProjection[cascade]);
    float3 ndc = positionLightSpace.xyz / positionLightSpace.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 1.0f - (ndc.y * 0.5f + 0.5f));

    float shadow = 0.0;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * lightDataCB.shadowMapTexelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, float3(uv + offset, cascade), ndc.z - lightDataCB.shadowBias);
        }
    }

    return shadow / 9.0f;
}

float ComputeShadow(float3 worldPos, Texture2DArray shadowMap)
{
    if (!lightDataCB.shadowEnabled || lightDataCB.cascadeCount == 0)
        return 1.0f;

    float viewDepth = dot(worldPos - sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward);
    uint cascade = SelectCascade(viewDepth);

    float split = cascade == 0 ? lightDataCB.cascadeSplits.x :
                  cascade == 1 ? lightDataCB.cascadeSplits.y :
                  cascade == 2 ? lightDataCB.cascadeSplits.z : 1e30f;
    float blendStart = split * 0.8f;
    float blend = saturate((viewDepth - blendStart) / max(split - blendStart, 1e-6f));

    float shadow = SampleCascadeShadow(worldPos, shadowMap, cascade);
    if (cascade < lightDataCB.cascadeCount - 1)
        shadow = lerp(shadow, SampleCascadeShadow(worldPos, shadowMap, cascade + 1), blend);

    return shadow;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    Texture2D diffuseMap = ResourceDescriptorHeap[constantsCB.diffuseIndex];
    Texture2D specularMap = ResourceDescriptorHeap[constantsCB.specularIndex];
    Texture2D normalMap = ResourceDescriptorHeap[constantsCB.normalIndex];
    Texture2DArray shadowMap = ResourceDescriptorHeap[constantsCB.shadowIndex];

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

    float3 L = normalize(sceneInfoCB.directionalLight.direction);
    float3 V = normalize(sceneInfoCB.cameraPosition - input.worldPos);
    float3 directionalLightColor = sceneInfoCB.directionalLight.color * sceneInfoCB.directionalLight.intensity;
    float NdotL = saturate(dot(worldNormal, -L));

    float shadow = ComputeShadow(input.worldPos, shadowMap);

    float3 ambient = 0.3f * diffuseTex.xyz * directionalLightColor;
    float3 diffuse = diffuseTex.xyz * NdotL * directionalLightColor;

    float3 H = normalize(V - L);
    float specFactor = (NdotL > 0.0f) ? pow(saturate(dot(N, H)), 64.0f) : 0.0f;
    float3 specular = specularTex.xyz * specFactor * directionalLightColor;

    float3 color = ambient + (diffuse + specular) * shadow;
    return float4(color, diffuseTex.w);
}
