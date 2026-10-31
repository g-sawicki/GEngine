#include "Interop/Common.h"
#include "Interop/SSAO.h"
#include "math.hlsli"

ConstantBuffer<SSAOConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<SampleData> samplesDataCB : register(b2);
SamplerState wrapSampler : register(s0);

[shader("compute")]
[numthreads(8, 8, 1)]
void SSAOCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 pixelCoords = dispatchThreadId.xy;
    if (any(pixelCoords >= sceneInfoCB.screenResolution))
        return;

    Texture2D<float3> normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[constantsCB.DepthIndex];
    Texture2D<float3> randomVectorTexture = ResourceDescriptorHeap[constantsCB.RandomVectorIndex];
    RWTexture2D<float> ambientOcclusionTexture = ResourceDescriptorHeap[constantsCB.AmbientOcclusionIndex];

    const float depth = depthTexture[pixelCoords];
    const float2 uv = CenterBiasedUVFromPixelCoordinate(pixelCoords, sceneInfoCB.screenResolution);
    const float3 viewPos = DepthReprojection(depth, uv, sceneInfoCB.inverseProjection);

    const float2 noiseUV = uv * (float2(sceneInfoCB.screenResolution) / kRandomVectorTextureSize);
    const float3 randomVector = randomVectorTexture.SampleLevel(wrapSampler, noiseUV, 0.0f);

    const float3 worldNormal = normalTexture[pixelCoords] * 2.0f - 1.0f;
    const float3 viewNormal = normalize(mul(worldNormal, (float3x3)sceneInfoCB.view));
    const float3 tangent = normalize(randomVector - viewNormal * dot(randomVector, viewNormal));
    const float3 bitangent = cross(viewNormal, tangent);
    const float3x3 TBN = float3x3(tangent, bitangent, viewNormal);

    float occlusion = 0.0f;
    for (uint sampleIndex = 0; sampleIndex < constantsCB.SampleCount; ++sampleIndex) {
        const float3 sampleOffset = samplesDataCB.SampleOffsets[sampleIndex].xyz;
        const float3 samplePosition = viewPos + mul(sampleOffset, TBN) * constantsCB.SampleRadius;

        float4 clipPosition = mul(float4(samplePosition, 1.0f), sceneInfoCB.projection);
        clipPosition.xyz /= clipPosition.w;
        float2 sampleUV = float2(clipPosition.x, -clipPosition.y) * 0.5f + 0.5f;
        const int2 samplePixel = clamp(int2(sampleUV * float2(sceneInfoCB.screenResolution)), int2(0, 0),
                                       int2(sceneInfoCB.screenResolution) - 1);
        sampleUV = CenterBiasedUVFromPixelCoordinate(uint2(samplePixel), sceneInfoCB.screenResolution);

        const float sampleDepth = depthTexture.Load(int3(samplePixel, 0));
        const float3 sampleViewPos = DepthReprojection(sampleDepth, sampleUV, sceneInfoCB.inverseProjection);

        const float depthDelta = samplePosition.z - sampleViewPos.z;
        occlusion +=
            (depthDelta > constantsCB.DepthBias && depthDelta < constantsCB.SampleRadius) ? 1.0f : 0.0f;
    }

    ambientOcclusionTexture[pixelCoords] = 1.0f - occlusion / constantsCB.SampleCount;
}