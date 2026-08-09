#pragma once

#include <d3d12.h>

namespace GEngine {

class Buffer;
class MeshBuffer;

struct MaterialGPU {
    D3D12_GPU_DESCRIPTOR_HANDLE DiffuseSRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE SpecularSRV{};
    D3D12_GPU_DESCRIPTOR_HANDLE NormalSRV{};
};

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* TransformCB{};
    MaterialGPU Material{};
    bool ShadowCaster{true};
};

} // namespace GEngine
