#include "common.hlsli"

struct VSInput {
    float4 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 worldPos : WORLDPOS;
    float3 tangent : TANGENT;
    float3 normal : NORMAL;
};

struct RootConstants {
    uint32_t diffuseIndex;
    uint32_t specularIndex;
    uint32_t normalIndex;
    uint32_t shadowIndex;
};

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<CascadedShadowMapsData> csmDataCB : register(b1);
ConstantBuffer<ObjectConstants> objectConstantsCB : register(b2);
ConstantBuffer<RootConstants> constantsCB : register(b3);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    float4 worldPos = mul(input.position, objectConstantsCB.world);

    PSInput output;
    output.position = mul(worldPos, sceneInfoCB.viewProjection);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), objectConstantsCB.world).xyz);
    output.normal = normalize(mul(float4(input.normal, 0.0f), objectConstantsCB.world).xyz);
    return output;
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
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
    float3 V = normalize(sceneInfoCB.cameraPosition - input.worldPos);

    float shadow = ComputeShadow(csmDataCB, shadowSampler, sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward,
                                 input.worldPos, shadowMap);

    float3 ambient = diffuseTex.xyz * 0.3;
    float3 directLighting = CalculateDirectLighting(sceneInfoCB.lightIndex, sceneInfoCB.lightCount, input.worldPos, V, worldNormal,
                                                    diffuseTex.xyz, specularTex.xyz, shadow);

    return float4(ambient + directLighting, diffuseTex.w);
}
