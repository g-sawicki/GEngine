#include "PCH.hpp"

#include "ForwardLighting.hpp"

#include "Rendering/MeshBuffer.hpp"

#include "default_ps.h"
#include "default_vs.h"

namespace GEngine::RenderPass {

ForwardLighting::ForwardLighting(Device& device, DXGI_FORMAT renderTargetFormat)
    : m_RenderTargetFormat(renderTargetFormat) {
    CD3DX12_ROOT_PARAMETER rootParams[2];
    rootParams[0].InitAsConstantBufferView(0);
    rootParams[1].InitAsConstantBufferView(1);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = m_RootSignature->Get(),
        .VS = {g_VSMain, sizeof(g_VSMain)},
        .PS = {g_PSMain, sizeof(g_PSMain)},
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {m_RenderTargetFormat},
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

ForwardLighting::~ForwardLighting() = default;

void ForwardLighting::OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, const SceneInfo& sceneInfo,
                               Buffer& sceneInfoConstantBuffer, std::span<const RenderItem> renderItems) {
    sceneInfoConstantBuffer.Write(&sceneInfo, sizeof(sceneInfo));

    auto* cmdList = commandList.GetHandle();
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());
    cmdList->SetGraphicsRootConstantBufferView(0, sceneInfoConstantBuffer.GetGPUVirtualAddress());
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    for (const auto& item : renderItems) {
        cmdList->SetGraphicsRootConstantBufferView(1, item.ObjectCB->GetGPUVirtualAddress());
        item.Mesh->Draw(commandList);
    }
}

} // namespace GEngine::RenderPass
