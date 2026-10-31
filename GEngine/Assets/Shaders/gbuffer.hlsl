#include "Interop/Common.h"
#include "Interop/GBuffer.h"

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
};

struct GBufferRenderTargets {
    float4 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 RoughnessMetallic : SV_TARGET2;
};

ConstantBuffer<GBufferConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<ObjectData> objectDataCB : register(b2);
SamplerState texSampler : register(s0);

[shader("vertex")]
PSInput VSMain(VSInput input) {
    float4 worldPos = mul(float4(input.position, 1.0f), objectDataCB.world);

    PSInput output;
    output.position = mul(worldPos, sceneInfoCB.viewProjection);
    output.normal = normalize(mul(float4(input.normal, 0.0f), objectDataCB.normal).xyz);
    output.tangent = normalize(mul(float4(input.tangent, 0.0f), objectDataCB.world).xyz);
    output.uv = input.uv;
    return output;
}

[shader("pixel")]
GBufferRenderTargets PSMain(PSInput input) {
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[constantsCB.AlbedoIndex];
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D<float3> roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.RoughnessMetallicIndex];

    const float4 albedo = albedoTexture.Sample(texSampler, input.uv);
    if (albedo.a < 0.01f)
        discard;
    const float3 normalTS = normalTexture.Sample(texSampler, input.uv) * 2.0f - 1.0f;
    const float2 roughnessMetallic = roughnessMetallicTexture.Sample(texSampler, input.uv).gb;

    const float3 N = normalize(input.normal);
    const float3 T = normalize(input.tangent - dot(input.tangent, N) * N);
    const float3 B = cross(N, T);
    const float3x3 TBN = float3x3(T, B, N);

    const float3 normal = normalize(mul(normalTS, TBN));

    GBufferRenderTargets output;
    output.Albedo = albedo;
    output.Normal = float4(normal * 0.5f + 0.5f, 0.0f);
    output.RoughnessMetallic = roughnessMetallic;
    return output;
}
