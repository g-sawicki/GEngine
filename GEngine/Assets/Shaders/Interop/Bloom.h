#ifndef SHADER_INTEROP_BLOOM_H
#define SHADER_INTEROP_BLOOM_H

#include "Interop.h"

struct BloomConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
    uint32_t Horizontal;
};

#endif // SHADER_INTEROP_BLOOM_H
