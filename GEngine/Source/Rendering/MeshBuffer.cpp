#include "PCH.hpp"

#include "MeshBuffer.hpp"

namespace GEngine {

MeshBuffer::MeshBuffer(Device& device, const Mesh& mesh)
    : m_VertexStride(static_cast<UINT>(sizeof(mesh.Vertices[0]))),
      m_IndexCount(static_cast<UINT>(mesh.Indices.size())) {
    m_VertexBuffer = std::make_unique<Buffer>(device, mesh.Vertices.size() * m_VertexStride, mesh.Vertices.data());
    m_IndexBuffer =
        std::make_unique<Buffer>(device, mesh.Indices.size() * sizeof(mesh.Indices[0]), mesh.Indices.data());
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
