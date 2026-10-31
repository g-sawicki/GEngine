#include "Interop/Common.h"
#include "Interop/ForwardLighting.h"
#include "light.hlsli"

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 worldPos : TEXCOORD3;
};

ConstantBuffer<ForwardLightingConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<CascadedShadowMapsData> csmDataCB : register(b2);
ConstantBuffer<ObjectData> objectDataCB : register(b3);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    float4 worldPos = mul(float4(input.position, 1.0f), objectDataCB.world);

    PSInput output;
    output.position = mul(worldPos, sceneInfoCB.viewProjection);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), objectDataCB.world).xyz);
    output.normal = normalize(mul(float4(input.normal, 0.0f), objectDataCB.normal).xyz);
    return output;
}

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[constantsCB.AlbedoIndex];
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D<float3> roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.RoughnessMetallicIndex];
    Texture2DArray shadowTexture = ResourceDescriptorHeap[constantsCB.ShadowIndex];

    float4 albedo = albedoTexture.Sample(texSampler, input.uv);
    if (albedo.a < 0.01f)
        discard;
    const float3 normalTS = normalTexture.Sample(texSampler, input.uv) * 2.0f - 1.0f;
    const float2 roughnessMetallic = roughnessMetallicTexture.Sample(texSampler, input.uv).gb;
    const float roughness = roughnessMetallic.x;
    const float metallic = roughnessMetallic.y;

    const float3 N = normalize(input.normal);
    const float3 T = normalize(input.tangent - dot(input.tangent, N) * N);
    const float3 B = cross(N, T);
    const float3x3 TBN = float3x3(T, B, N);

    const float3 normal = normalize(mul(normalTS, TBN));
    const float3 V = normalize(sceneInfoCB.cameraPosition - input.worldPos);

    Material material;
    material.albedo = albedo.rgb;
    material.roughness = roughness;
    material.metallic = metallic;

    float shadow = ComputeShadow(csmDataCB, shadowSampler, sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward,
                                 input.worldPos, shadowTexture);

    float3 indirectLighting = CalculateIndirectLighting(sceneInfoCB.irradianceIndex, texSampler, normal, V, material);

    float3 directLighting = CalculateDirectLighting(sceneInfoCB.lightIndex, sceneInfoCB.lightCount, input.worldPos, V, normal,
                                                    material, shadow);

    return float4(indirectLighting + directLighting, albedo.w);
}
