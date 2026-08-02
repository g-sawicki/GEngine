#pragma once

namespace GEngine {

class Buffer;
class MeshBuffer;

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* ObjectCB{};
};

} // namespace GEngine
