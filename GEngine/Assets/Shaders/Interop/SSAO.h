#ifndef SHADER_INTEROP_SSAO_H
#define SHADER_INTEROP_SSAO_H

#include "Interop.h"

static const uint32_t kMaxSamples = 64;
static const uint32_t kRandomVectorTextureSize = 4;

struct SSAOConstants {
    uint32_t NormalIndex;
    uint32_t DepthIndex;
    uint32_t RandomVectorIndex;
    uint32_t AmbientOcclusionIndex;
    uint32_t SampleCount;
    float SampleRadius;
    float DepthBias;
};

struct SampleData {
    float4 SampleOffsets[kMaxSamples];
};

#endif // SHADER_INTEROP_SSAO_H
