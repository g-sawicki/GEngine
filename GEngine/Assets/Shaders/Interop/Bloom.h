#ifndef SHADER_INTEROP_BLOOM_H
#define SHADER_INTEROP_BLOOM_H

#include "Interop.h"

static const uint32_t kMaxBloomOffset = 64;

struct BloomConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
    uint32_t Horizontal;
    uint32_t WeightCount;
    float Threshold;
    float Intensity;
};

struct BloomWeights {
    float4 Weights[kMaxBloomOffset / 4];
};

#endif // SHADER_INTEROP_BLOOM_H
