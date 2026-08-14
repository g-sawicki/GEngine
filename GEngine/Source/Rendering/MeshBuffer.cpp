#include "PCH.hpp"

#include "MeshBuffer.hpp"

namespace GEngine {

MeshBuffer::MeshBuffer(Device& device, CommandQueue& commandQueue, const Mesh& mesh)
    : m_VertexStride(static_cast<UINT>(sizeof(mesh.Vertices[0]))),
      m_IndexCount(static_cast<UINT>(mesh.Indices.size())) {

    const BufferDesc vertexBufferDesc{
        .Size = mesh.Vertices.size() * m_VertexStride,
        .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    };
    m_VertexBuffer = Buffer{device, commandQueue, vertexBufferDesc, mesh.Vertices.data()};

    const BufferDesc indexBufferDesc{
        .Size = mesh.Indices.size() * sizeof(mesh.Indices[0]),
        .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    };
    m_IndexBuffer = Buffer{device, commandQueue, indexBufferDesc, mesh.Indices.data()};
}

void MeshBuffer::Draw(CommandList& commandList) const {
    auto* cmdList = commandList.GetHandle();

    auto vbv{m_VertexBuffer.GetVBV(m_VertexStride)};
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    auto ibv{m_IndexBuffer.GetIBV()};
    cmdList->IASetIndexBuffer(&ibv);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

} // namespace GEngine
