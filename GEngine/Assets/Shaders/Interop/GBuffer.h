#ifndef SHADER_INTEROP_GBUFFER_H
#define SHADER_INTEROP_GBUFFER_H

#include "Interop.h"

struct GBufferConstants {
    uint32_t AlbedoIndex;
    uint32_t NormalIndex;
    uint32_t RoughnessMetallicIndex;
};

#endif // SHADER_INTEROP_GBUFFER_H
