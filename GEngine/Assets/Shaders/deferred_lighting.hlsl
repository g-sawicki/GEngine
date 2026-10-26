#include "Interop/Common.h"
#include "Interop/DeferredLighting.h"
#include "Interop/GBuffer.h"
#include "light.hlsli"

ConstantBuffer<DeferredLightingConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<CascadedShadowMapsData> csmDataCB : register(b2);
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
    Texture2D ambientOcclusionTexture = ResourceDescriptorHeap[constantsCB.AmbientOcclusionIndex];
    Texture2D depthTexture = ResourceDescriptorHeap[constantsCB.DepthIndex];
    Texture2DArray shadowTexture = ResourceDescriptorHeap[constantsCB.ShadowIndex];
    RWTexture2D<float4> hdrTexture = ResourceDescriptorHeap[constantsCB.HdrIndex];

    int2 pixelCoords = dispatchThreadId.xy;
    int3 loadLocation = int3(dispatchThreadId.xy, 0);
    float2 uv = CenterBiasedUVFromPixelCoordinate(pixelCoords, sceneInfoCB.screenResolution);

    float3 albedo = albedoTexture.Load(loadLocation).rgb;
    float3 normal = normalize(normalTexture.Load(loadLocation).xyz * 2.0f - 1.0f);
    float2 roughnessMetallic = roughnessMetallicTexture.Load(loadLocation).rg;
    float ambientOcclusion = ambientOcclusionTexture.Load(loadLocation).r;
    float depth = depthTexture.Load(loadLocation).r;

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
