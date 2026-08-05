#include "PCH.hpp"

#include "ForwardLightingPass.hpp"

#include "Rendering/MeshBuffer.hpp"

#include "default_ps.h"
#include "default_vs.h"

namespace GEngine::RenderPass {

ForwardLightingPass::ForwardLightingPass(Device& device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat)
    : m_RenderTargetFormat(renderTargetFormat), m_DepthStencilFormat(depthStencilFormat) {
    CD3DX12_ROOT_PARAMETER rootParams[3]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: SceneInfo
    rootParams[1].InitAsConstantBufferView(1); // b1: ObjectConstants

    CD3DX12_DESCRIPTOR_RANGE textureRanges[2]{};
    textureRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0: diffuse
    textureRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1: specular

    rootParams[2].InitAsDescriptorTable(2, textureRanges);

    D3D12_STATIC_SAMPLER_DESC staticSampler{
        .Filter = D3D12_FILTER_ANISOTROPIC,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
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
        .DSVFormat = m_DepthStencilFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

ForwardLightingPass::~ForwardLightingPass() = default;

void ForwardLightingPass::OnRender(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv,
                                   D3D12_CPU_DESCRIPTOR_HANDLE dsv, const SceneInfo& sceneInfo,
                                   Buffer& sceneInfoConstantBuffer, std::span<const RenderItem> renderItems) {
    sceneInfoConstantBuffer.Write(&sceneInfo, sizeof(sceneInfo));

    auto* cmdList = commandList.GetHandle();
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());
    cmdList->SetGraphicsRootConstantBufferView(0, sceneInfoConstantBuffer.GetGPUVirtualAddress());
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    for (const auto& item : renderItems) {
        cmdList->SetGraphicsRootConstantBufferView(1, item.ObjectCB->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(2, item.MaterialSRV);
        item.Mesh->Draw(commandList);
    }
}

} // namespace GEngine::RenderPass
