#ifndef SHADER_INTEROP_GAUSSIAN_BLUR_H
#define SHADER_INTEROP_GAUSSIAN_BLUR_H

#include "Interop.h"

static const uint32_t kMaxOffset = 64;

struct GaussianBlurConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
    uint32_t Horizontal;
    uint32_t WeightCount;
};

struct GaussianWeights {
    float4 Weights[kMaxOffset / 4];
};

#endif // SHADER_INTEROP_GAUSSIAN_BLUR_H
