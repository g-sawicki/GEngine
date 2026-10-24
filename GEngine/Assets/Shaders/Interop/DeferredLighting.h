#ifndef SHADER_INTEROP_DEFERRED_LIGHTING_H
#define SHADER_INTEROP_DEFERRED_LIGHTING_H

struct DeferredLightingConstants {
    uint32_t AlbedoIndex;
    uint32_t NormalIndex;
    uint32_t RoughnessMetallicIndex;
    uint32_t DepthIndex;
    uint32_t ShadowIndex;
    uint32_t HdrIndex;
};

#endif // SHADER_INTEROP_DEFERRED_LIGHTING_H
