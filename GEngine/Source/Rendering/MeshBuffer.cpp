#include "PCH.hpp"

#include "MeshBuffer.hpp"

namespace GEngine {

MeshBuffer::MeshBuffer(Device& device, const Mesh& mesh)
    : m_VertexStride(static_cast<UINT>(sizeof(mesh.vertices[0]))),
      m_IndexCount(static_cast<UINT>(mesh.indices.size())) {
    m_VertexBuffer = std::make_unique<Buffer>(device, mesh.vertices.size() * m_VertexStride, mesh.vertices.data());
    m_IndexBuffer =
        std::make_unique<Buffer>(device, mesh.indices.size() * sizeof(mesh.indices[0]), mesh.indices.data());
}

void MeshBuffer::Draw(CommandList& commandList) const {
    auto* cmdList = commandList.GetHandle();

    auto vbv{m_VertexBuffer->GetVBV(m_VertexStride)};
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    auto ibv{m_IndexBuffer->GetIBV()};
    cmdList->IASetIndexBuffer(&ibv);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

} // namespace GEngine
