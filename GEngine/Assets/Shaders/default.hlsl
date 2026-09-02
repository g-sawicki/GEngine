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
    uint32_t albedoIndex;
    uint32_t normalIndex;
    uint32_t roughnessMetallicIndex;
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
    Texture2D albedoTexture = ResourceDescriptorHeap[constantsCB.albedoIndex];
    Texture2D normalTexture = ResourceDescriptorHeap[constantsCB.normalIndex];
    Texture2D roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.roughnessMetallicIndex];
    Texture2DArray shadowTexture = ResourceDescriptorHeap[constantsCB.shadowIndex];

    float4 albedo = albedoTexture.Sample(texSampler, input.uv);
    if (albedo.a < 0.01f)
        discard;
    float3 normalTS = (normalTexture.Sample(texSampler, input.uv).xyz * 2.0f - 1.0f);
    float4 roughnessMetallic = roughnessMetallicTexture.Sample(texSampler, input.uv);
    float roughness = roughnessMetallic.y;
    float metallic = roughnessMetallic.z;

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz - dot(input.tangent.xyz, N) * N);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    float3 normal = normalize(mul(normalTS, TBN));
    float3 V = normalize(sceneInfoCB.cameraPosition - input.worldPos);

    Material material;
    material.albedo = albedo;
    material.roughness = roughness;
    material.metallic = metallic;

    float shadow = ComputeShadow(csmDataCB, shadowSampler, sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward,
                                 input.worldPos, shadowTexture);

    float3 ambient = albedo.xyz * 0.03;
    float3 directLighting = CalculateDirectLighting(sceneInfoCB.lightIndex, sceneInfoCB.lightCount, input.worldPos, V, normal,
                                                    material, shadow);

    return float4(ambient + directLighting, albedo.w);
}
