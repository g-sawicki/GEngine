#include "Interop/Common.h"
#include "Interop/GBuffer.h"
#include "light.hlsli"

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct GBufferRenderTargets {
    float4 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 RoughnessMetallic : SV_TARGET2;
};

ConstantBuffer<GBufferConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<ObjectData> objectDataCB : register(b2);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    float4 worldPos = mul(float4(input.position, 1.0f), objectDataCB.world);

    PSInput output;
    output.position = mul(worldPos, sceneInfoCB.viewProjection);
    output.normal = normalize(mul(float4(input.normal, 0.0f), objectDataCB.world).xyz);
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), objectDataCB.world).xyz);
    output.uv = input.uv;
    return output;
}

[shader("pixel")]
GBufferRenderTargets PSMain(PSInput input) {
    Texture2D albedoTexture = ResourceDescriptorHeap[constantsCB.AlbedoIndex];
    Texture2D normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.RoughnessMetallicIndex];

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

    GBufferRenderTargets output;
    output.Albedo = albedo;
    output.Normal = float4(normal * 0.5f + 0.5f, 0.0f);
    output.RoughnessMetallic = float4(roughness, metallic, 0.0f, 0.0f);
    return output;
}
