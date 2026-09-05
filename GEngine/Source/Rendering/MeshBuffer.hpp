#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Scene/Model.hpp"

namespace GEngine {

class MeshBuffer {
  public:
    MeshBuffer(Device& device, CommandQueue& uploadQueue, const Mesh& mesh);

    GE_NO_COPY_DEFAULT_MOVE(MeshBuffer)

    void Draw(CommandList& commandList) const;

    [[nodiscard]] Buffer& GetVertexBuffer() noexcept { return m_VertexBuffer; }
    [[nodiscard]] Buffer& GetIndexBuffer() noexcept { return m_IndexBuffer; }
    [[nodiscard]] UINT GetVertexStride() const noexcept { return m_VertexStride; }
    [[nodiscard]] UINT GetIndexCount() const noexcept { return m_IndexCount; }

  private:
    Buffer m_VertexBuffer{};
    Buffer m_IndexBuffer{};
    UINT m_VertexStride{};
    UINT m_IndexCount{};
};

} // namespace GEngine
