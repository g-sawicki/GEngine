#include "PCH.hpp"

#include "ShadowPass.hpp"

#include "Rendering/MeshBuffer.hpp"

#include "shadow_pass_ps.h"
#include "shadow_pass_vs.h"

namespace GEngine::RenderPass {

ShadowPass::ShadowPass(Device& device, DXGI_FORMAT depthStencilFormat) : m_DepthStencilFormat(depthStencilFormat) {
    CD3DX12_ROOT_PARAMETER rootParams[3]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: LightDataConstants
    rootParams[1].InitAsConstantBufferView(1); // b1: ObjectConstants
    rootParams[2].InitAsConstants(1, 2);       // b2: CascadeIndex (uint)

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
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
        .NumRenderTargets = 0,
        .RTVFormats = {},
        .DSVFormat = m_DepthStencilFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

ShadowPass::~ShadowPass() = default;

void ShadowPass::OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, Buffer& lightDataConstantBuffer,
                          uint32_t cascadeIndex, std::span<const RenderItem> renderItems) {
    auto* cmdList = commandList.GetHandle();
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());

    cmdList->SetGraphicsRootConstantBufferView(0, lightDataConstantBuffer.GetGPUVirtualAddress());
    cmdList->SetGraphicsRoot32BitConstant(2, cascadeIndex, 0);

    for (const auto& item : renderItems) {
        if (!item.ShadowCaster)
            continue;
        cmdList->SetGraphicsRootConstantBufferView(1, item.TransformCB->GetGPUVirtualAddress());
        item.Mesh->Draw(commandList);
    }
}

} // namespace GEngine::RenderPass
