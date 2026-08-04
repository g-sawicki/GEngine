#pragma once

#include <d3d12.h>

namespace GEngine {

class Buffer;
class MeshBuffer;

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* ObjectCB{};
    D3D12_GPU_DESCRIPTOR_HANDLE MaterialSRV{};
};

} // namespace GEngine
