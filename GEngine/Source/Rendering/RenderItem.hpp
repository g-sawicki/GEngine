#pragma once

#include <d3d12.h>

namespace GEngine {

class Buffer;
class MeshBuffer;

struct Material {
    D3D12_GPU_DESCRIPTOR_HANDLE DiffuseSRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE SpecularSRV{};
};

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* TransformCB{};
    const Material Material;
};

} // namespace GEngine
