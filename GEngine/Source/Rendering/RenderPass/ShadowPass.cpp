#include "PCH.hpp"

#include "ShadowPass.hpp"

#include "Graphics/D3D12/Shader.hpp"
#include "Rendering/MeshBuffer.hpp"

namespace GEngine::RenderPass {

ShadowPass::ShadowPass(Device& device, DXGI_FORMAT depthStencilFormat) : m_DepthStencilFormat(depthStencilFormat) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: LightDataConstants
    rootParams[1].InitAsConstantBufferView(1); // b1: ObjectConstants
    rootParams[2].InitAsConstants(1, 2);       // b2: RootConstants

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    const Shader vertexShader{"Assets/Shaders/shadow_pass_vs.cso"};
    const Shader pixelShader{"Assets/Shaders/shadow_pass_ps.cso"};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = m_RootSignature->Get(),
        .VS = vertexShader.GetBytecode(),
        .PS = pixelShader.GetBytecode(),
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

void ShadowPass::OnRender(CommandList& commandList, Texture& shadowMapTexture, Buffer& lightDataConstantBuffer,
                          uint32_t cascadeCount, std::span<const RenderItem> renderItems) {
    auto* cmdList = commandList.GetHandle();
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());
    cmdList->SetGraphicsRootConstantBufferView(0, lightDataConstantBuffer.GetGPUVirtualAddress());

    for (uint8_t cascadeIndex{}; cascadeIndex < cascadeCount; ++cascadeIndex) {
        const D3D12_CPU_DESCRIPTOR_HANDLE depthDsv = shadowMapTexture.GetDsvHandle(cascadeIndex);
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &depthDsv);
        const D3D12_CLEAR_VALUE& clearValue = shadowMapTexture.GetDesc().ClearValue;
        cmdList->ClearDepthStencilView(depthDsv, D3D12_CLEAR_FLAG_DEPTH, clearValue.DepthStencil.Depth,
                                       clearValue.DepthStencil.Stencil, 0, nullptr);
        cmdList->SetGraphicsRoot32BitConstant(2, cascadeIndex, 0);
        for (const auto& item : renderItems) {
            if (!item.ShadowCaster)
                continue;
            cmdList->SetGraphicsRootConstantBufferView(1, item.TransformCB->GetGPUVirtualAddress());
            item.Mesh->Draw(commandList);
        }
    }
}

} // namespace GEngine::RenderPass
