#ifndef SHADER_INTEROP_TONEMAP_H
#define SHADER_INTEROP_TONEMAP_H

#include "Interop.h"

enum ToneMapMode : uint32_t {
    Reinhard = 0,
    ExtendedReinhard = 1,
    Uncharted2 = 2,
    ACES = 3,
};

struct ToneMapConstants {
    uint32_t InputIndex;
    uint32_t OutputIndex;
    ToneMapMode TonemapMode;
    float MaxWhite; // Only used for ExtendedReinhard mode
};

#endif // SHADER_INTEROP_TONEMAP_H
