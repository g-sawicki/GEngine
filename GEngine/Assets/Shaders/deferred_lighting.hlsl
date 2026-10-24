#include "Interop/Common.h"
#include "Interop/DeferredLighting.h"
#include "Interop/GBuffer.h"
#include "light.hlsli"

ConstantBuffer<SceneInfo> sceneInfoCB : register(b0);
ConstantBuffer<CascadedShadowMapsData> csmDataCB : register(b1);
ConstantBuffer<DeferredLightingConstants> constantsCB : register(b2);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

[shader("compute")]
[numthreads(8, 8, 1)]
void DeferredLightingCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    if (dispatchThreadId.x >= sceneInfoCB.screenResolution.x || dispatchThreadId.y >= sceneInfoCB.screenResolution.y)
        return;

    Texture2D albedoTexture = ResourceDescriptorHeap[constantsCB.AlbedoIndex];
    Texture2D normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.RoughnessMetallicIndex];
    Texture2D depthTexture = ResourceDescriptorHeap[constantsCB.DepthIndex];
    Texture2DArray shadowTexture = ResourceDescriptorHeap[constantsCB.ShadowIndex];
    RWTexture2D<float4> hdrTexture = ResourceDescriptorHeap[constantsCB.HdrIndex];

    float2 uv = (float2(dispatchThreadId.xy) + 0.5f) / float2(sceneInfoCB.screenResolution);

    float depth = depthTexture.Load(int3(dispatchThreadId.xy, 0)).r;
    float3 ndc = float3(uv * 2.0f - 1.0f, depth);
    ndc.y = -ndc.y;
    float4 worldH = mul(float4(ndc, 1.0f), sceneInfoCB.inverseViewProjection);
    float3 worldPos = worldH.xyz / worldH.w;

    float3 normal = normalize(normalTexture.Sample(texSampler, uv).xyz * 2.0f - 1.0f);

    Material material;
    material.albedo = albedoTexture.Sample(texSampler, uv).rgb;
    material.roughness = roughnessMetallicTexture.Sample(texSampler, uv).r;
    material.metallic = roughnessMetallicTexture.Sample(texSampler, uv).g;

    float3 V = normalize(sceneInfoCB.cameraPosition - worldPos);

    float shadow = ComputeShadow(csmDataCB, shadowSampler, sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward,
                                 worldPos, shadowTexture);

    float3 indirectLighting = CalculateIndirectLighting(sceneInfoCB.irradianceIndex, texSampler, normal, V, material);

    float3 directLighting = CalculateDirectLighting(sceneInfoCB.lightIndex, sceneInfoCB.lightCount, worldPos, V, normal,
                                                    material, shadow);

    float3 color = indirectLighting + directLighting;
    hdrTexture[dispatchThreadId.xy] = float4(color, 1.0f);
}
