#ifndef SHADER_INTEROP_FORWARD_LIGHTING_H
#define SHADER_INTEROP_FORWARD_LIGHTING_H

#include "Interop.h"

struct ForwardLightingConstants {
    uint32_t AlbedoIndex;
    uint32_t NormalIndex;
    uint32_t RoughnessMetallicIndex;
    uint32_t ShadowIndex;
};

#endif // SHADER_INTEROP_FORWARD_LIGHTING_H
