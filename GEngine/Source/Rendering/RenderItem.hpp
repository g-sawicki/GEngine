#pragma once

#include <d3d12.h>

namespace GEngine {

class Buffer;
class MeshBuffer;

struct MaterialGPU {
    uint32_t DiffuseIndex{};
    uint32_t SpecularIndex{};
    uint32_t NormalIndex{};
};

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* TransformCB{};
    MaterialGPU Material{};
    bool ShadowCaster{true};
};

} // namespace GEngine
