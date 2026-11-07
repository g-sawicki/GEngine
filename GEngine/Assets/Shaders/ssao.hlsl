#include "Interop/Common.h"
#include "Interop/SSAO.h"
#include "math.hlsli"

#define KERNEL_SIZE 16

ConstantBuffer<SSAOConstants> constantsCB : register(b0);
ConstantBuffer<SceneInfo> sceneInfoCB : register(b1);
ConstantBuffer<SampleData> samplesDataCB : register(b2);
SamplerState wrapSampler : register(s0);

[shader("compute")]
[numthreads(8, 8, 1)]
void SSAOCS(uint3 dispatchThreadId : SV_DispatchThreadID) {
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[constantsCB.NormalIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[constantsCB.DepthIndex];
    Texture2D<float3> randomVectorTexture = ResourceDescriptorHeap[constantsCB.RandomVectorIndex];
    RWTexture2D<float> ambientOcclusionTexture = ResourceDescriptorHeap[constantsCB.AmbientOcclusionIndex];

    uint2 resolution;
    ambientOcclusionTexture.GetDimensions(resolution.x, resolution.y);
    const float2 uv = GetCenterBiasedUV(dispatchThreadId.xy, resolution);

    const uint2 inputPixelCoord = uint2(uv * float2(sceneInfoCB.screenResolution));
    const float3 worldNormal = normalize(normalTexture[inputPixelCoord] * 2.0f - 1.0f);
    const float depth = depthTexture[inputPixelCoord];

    const float3 viewPosition = DepthReprojection(depth, uv, sceneInfoCB.inverseProjection);

    const float2 noiseUV = uv * resolution / float(kRandomVectorTextureSize);
    const float3 randomVector = randomVectorTexture.SampleLevel(wrapSampler, noiseUV, 0.0f);

    const float3 viewNormal = normalize(mul(worldNormal, (float3x3)sceneInfoCB.view));
    
    const float3 tangent = normalize(randomVector - viewNormal * dot(randomVector, viewNormal));
    const float3 bitangent = cross(viewNormal, tangent);
    const float3x3 TBN = float3x3(tangent, bitangent, viewNormal);

    float occlusion = 0.0f;
    [unroll]
    for (uint i = 0; i < KERNEL_SIZE; ++i) {
        const float3 sampleOffset = mul(samplesDataCB.SampleOffsets[i].xyz, TBN);
        const float3 samplePosition = viewPosition + sampleOffset * constantsCB.SampleRadius;

        float4 clipPosition = mul(float4(samplePosition, 1.0f), sceneInfoCB.projection);
        clipPosition.xy /= clipPosition.w;
        float2 sampleUV = float2(clipPosition.x, -clipPosition.y) * 0.5f + 0.5f;
        const int2 samplePixel = int2(sampleUV * float2(sceneInfoCB.screenResolution));
        float sampleDepth = depthTexture.Load(int3(samplePixel, 0));
        sampleDepth = DepthReprojection(sampleDepth, sampleUV, sceneInfoCB.inverseProjection).z;
        float rangeCheck = smoothstep(0.0, 1.0, constantsCB.SampleRadius / abs(viewPosition.z - sampleDepth));
        occlusion += rangeCheck * step(sampleDepth, samplePosition.z);
    }

    ambientOcclusionTexture[dispatchThreadId.xy] = 1.0f - occlusion / KERNEL_SIZE;
}