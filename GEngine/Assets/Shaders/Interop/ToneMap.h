#ifndef SHADER_INTEROP_TONEMAP_H
#define SHADER_INTEROP_TONEMAP_H

#include "Interop.h"

enum ToneMapMode : uint32_t {
    Reinhard = 0,
    ExtendedReinhard = 1,
    Uncharted2 = 2,
};

struct ToneMapRootConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
    uint32_t TonemapMode;
    float MaxWhite; // Only used for ExtendedReinhard mode
};

#endif // SHADER_INTEROP_TONEMAP_H
