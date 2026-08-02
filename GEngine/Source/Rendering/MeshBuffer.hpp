#pragma once

#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Rendering/Mesh.hpp"

namespace GEngine {

class MeshBuffer {
  public:
    MeshBuffer(Device& device, const Mesh& mesh);
    ~MeshBuffer() = default;

    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) = default;
    MeshBuffer& operator=(MeshBuffer&&) = default;

    void Draw(CommandList& commandList) const;

    [[nodiscard]] UINT GetIndexCount() const noexcept { return m_IndexCount; }

  private:
    std::unique_ptr<Buffer> m_VertexBuffer;
    std::unique_ptr<Buffer> m_IndexBuffer;
    UINT m_VertexStride{};
    UINT m_IndexCount{};
};

} // namespace GEngine
