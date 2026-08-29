#pragma once

#include <d3d12.h>

namespace GEngine {

class Buffer;
class MeshBuffer;

struct MaterialGPU {
    uint32_t AlbedoIndex{};
    uint32_t NormalIndex{};
    uint32_t RoughnessMetallicIndex{};
};

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* TransformCB{};
    MaterialGPU Material{};
    bool ShadowCaster{true};
};

} // namespace GEngine
