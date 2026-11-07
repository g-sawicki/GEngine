#include "Interop/Common.h"
#include "Interop/DeferredLighting.h"
#include "Interop/GBuffer.h"
#include "light.hlsli"

ConstantBuffer<DeferredLightingConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<CascadedShadowMapsData> csmDataCB : register(b2);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);
SamplerState ambientOcclusionSampler : register(s2);

[shader("compute")]
[numthreads(8, 8, 1)]
void DeferredLightingCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoords = dispatchThreadId.xy;
    if (any(pixelCoords >= sceneInfoCB.screenResolution))
        return;

    Texture2D<float3> albedoTexture = ResourceDescriptorHeap[constantsCB.AlbedoIndex];
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D<float2> roughnessMetallicTexture = ResourceDescriptorHeap[constantsCB.RoughnessMetallicIndex];
    Texture2D<float> ambientOcclusionTexture = ResourceDescriptorHeap[constantsCB.AmbientOcclusionIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[constantsCB.DepthIndex];
    Texture2DArray shadowTexture = ResourceDescriptorHeap[constantsCB.ShadowIndex];
    RWTexture2D<float4> hdrTexture = ResourceDescriptorHeap[constantsCB.HdrIndex];

    const float2 uv = GetCenterBiasedUV(pixelCoords, sceneInfoCB.screenResolution);

    const float3 albedo = albedoTexture[pixelCoords];
    const float3 normal = normalize(normalTexture[pixelCoords] * 2.0f - 1.0f);
    const float2 roughnessMetallic = roughnessMetallicTexture[pixelCoords];
    const float ambientOcclusion = ambientOcclusionTexture.SampleLevel(ambientOcclusionSampler, uv, 0);
    const float depth = depthTexture[pixelCoords];

    Material material;
    material.albedo = albedo;
    material.roughness = roughnessMetallic.r;
    material.metallic = roughnessMetallic.g;

    float3 worldPos = DepthReprojection(depth, uv, sceneInfoCB.inverseViewProjection);

    float3 V = normalize(sceneInfoCB.cameraPosition - worldPos);

    float shadow = ComputeShadow(csmDataCB, shadowSampler, sceneInfoCB.cameraPosition, sceneInfoCB.cameraForward,
                                 worldPos, shadowTexture);

    float3 indirectLighting = CalculateIndirectLighting(sceneInfoCB.irradianceIndex, texSampler, normal, V, material);

    float3 directLighting = CalculateDirectLighting(sceneInfoCB.lightIndex, sceneInfoCB.lightCount, worldPos, V, normal,
                                                    material, shadow);

    float3 color = indirectLighting * ambientOcclusion + directLighting;
    hdrTexture[dispatchThreadId.xy] = float4(color, 1.0f);
}
