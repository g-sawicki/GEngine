#include "PCH.hpp"

#include "SkyboxPass.hpp"

#include "Graphics/D3D12/Shader.hpp"

namespace GEngine::RenderPass {

SkyboxPass::SkyboxPass(Device& device, const Texture& colorTexture, const Texture& depthTexture) {
    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: SceneInfo
    rootParams[1].InitAsConstants(1, 1);       // b1: RootConstants

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
    staticSamplers[0] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 1,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .MinLOD = 0.0f,
        .MaxLOD = 0.0f,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = static_cast<UINT>(std::size(staticSamplers));
    rootSigDesc.pStaticSamplers = staticSamplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    const Shader vertexShader{"Assets/Shaders/skybox_vs.cso"};
    const Shader pixelShader{"Assets/Shaders/skybox_ps.cso"};

    D3D12_RASTERIZER_DESC rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    D3D12_DEPTH_STENCIL_DESC depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = m_RootSignature->Get(),
        .VS = vertexShader.GetBytecode(),
        .PS = pixelShader.GetBytecode(),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = rasterizerState,
        .DepthStencilState = depthStencilState,
        .InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {colorTexture.GetDesc().Format},
        .DSVFormat = depthTexture.GetDesc().Format,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

void SkyboxPass::OnRender(CommandList& commandList, const MeshBuffer& cubeMesh, const Texture& colorTexture,
                          const Texture& depthTexture, uint32_t skyboxSrvIndex, Buffer& sceneInfoCB) {
    auto* cmdList = commandList.GetHandle();

    D3D12_VIEWPORT viewport{
        0,    0,   static_cast<float>(colorTexture.GetDesc().Width), static_cast<float>(colorTexture.GetDesc().Height),
        0.0f, 1.0f};
    D3D12_RECT scissorRect{0, 0, static_cast<LONG>(colorTexture.GetDesc().Width),
                           static_cast<LONG>(colorTexture.GetDesc().Height)};
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    const D3D12_CPU_DESCRIPTOR_HANDLE colorRtv = colorTexture.GetRtvHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE depthDsv = depthTexture.GetDsvHandle();
    cmdList->OMSetRenderTargets(1, &colorRtv, FALSE, &depthDsv);
    cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());

    cmdList->SetGraphicsRootConstantBufferView(0, sceneInfoCB.GetGPUVirtualAddress());
    cmdList->SetGraphicsRoot32BitConstants(1, 1, &skyboxSrvIndex, 0);
    cubeMesh.Draw(commandList);
}

} // namespace GEngine::RenderPass
