#include "PCH.hpp"

#include "ForwardLightingPass.hpp"

#include "Rendering/MeshBuffer.hpp"

#include "default_ps.h"
#include "default_vs.h"

namespace GEngine::RenderPass {

ForwardLightingPass::ForwardLightingPass(Device& device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat)
    : m_RenderTargetFormat(renderTargetFormat), m_DepthStencilFormat(depthStencilFormat) {
    CD3DX12_ROOT_PARAMETER rootParams[7]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: SceneInfo
    rootParams[1].InitAsConstantBufferView(1); // b1: ObjectConstants
    rootParams[2].InitAsConstantBufferView(2); // b2: LightData

    CD3DX12_DESCRIPTOR_RANGE diffuseMapRange{};
    diffuseMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0: diffuse
    CD3DX12_DESCRIPTOR_RANGE specularMapRange{};
    specularMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1: specular
    CD3DX12_DESCRIPTOR_RANGE normalMapRange{};
    normalMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2: specular
    CD3DX12_DESCRIPTOR_RANGE shadowMapRange{};
    shadowMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // t3: shadow map

    rootParams[3].InitAsDescriptorTable(1, &diffuseMapRange);
    rootParams[4].InitAsDescriptorTable(1, &specularMapRange);
    rootParams[5].InitAsDescriptorTable(1, &normalMapRange);
    rootParams[6].InitAsDescriptorTable(1, &shadowMapRange);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
    staticSamplers[0] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_ANISOTROPIC,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };
    staticSamplers[1] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 1,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        .MinLOD = 0.0f,
        .MaxLOD = 0.0f,
        .ShaderRegister = 1,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = static_cast<UINT>(std::size(staticSamplers));
    rootSigDesc.pStaticSamplers = staticSamplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
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
                                   D3D12_CPU_DESCRIPTOR_HANDLE dsv, Buffer& sceneInfoConstantBuffer,
                                   Buffer& lightDataConstantBuffer, D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSRV,
                                   std::span<const RenderItem> renderItems) {

    auto* cmdList = commandList.GetHandle();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());

    cmdList->SetGraphicsRootConstantBufferView(0, sceneInfoConstantBuffer.GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, lightDataConstantBuffer.GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(6, shadowMapSRV);

    for (const auto& item : renderItems) {
        cmdList->SetGraphicsRootConstantBufferView(1, item.TransformCB->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(3, item.Material.DiffuseSRV);
        cmdList->SetGraphicsRootDescriptorTable(4, item.Material.SpecularSRV);
        cmdList->SetGraphicsRootDescriptorTable(5, item.Material.NormalSRV);
        item.Mesh->Draw(commandList);
    }
}

} // namespace GEngine::RenderPass
