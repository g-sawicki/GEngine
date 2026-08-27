#include "PCH.hpp"

#include "ForwardLightingPass.hpp"

#include "Graphics/D3D12/Shader.hpp"
#include "Rendering/MeshBuffer.hpp"

namespace GEngine::RenderPass {

ForwardLightingPass::ForwardLightingPass(Device& device, const Texture& colorTexture, const Texture& depthTexture) {
    CD3DX12_ROOT_PARAMETER1 rootParams[4]{};
    rootParams[0].InitAsConstantBufferView(0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);
    rootParams[3].InitAsConstants(4, 3);

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
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    const Shader vertexShader{"Assets/Shaders/default_vs.cso"};
    const Shader pixelShader{"Assets/Shaders/default_ps.cso"};

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
        .NumRenderTargets = 1,
        .RTVFormats = {colorTexture.GetDesc().Format},
        .DSVFormat = depthTexture.GetDesc().Format,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

void ForwardLightingPass::OnRender(CommandList& commandList, const Texture& colorTexture, const Texture& depthTexture,
                                   const Texture& shadowMapTexture, const Buffer& sceneInfoCB,
                                   const Buffer& cascadedShadowMapsDataCB, std::span<const RenderItem> renderItems) {

    auto* cmdList = commandList.GetHandle();
    cmdList->ClearRenderTargetView(colorTexture.GetRtvHandle(), colorTexture.GetDesc().ClearValue.Color, 0, nullptr);
    cmdList->ClearDepthStencilView(depthTexture.GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                   nullptr); // TODO: Use texture clear

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
    cmdList->SetGraphicsRootConstantBufferView(1, cascadedShadowMapsDataCB.GetGPUVirtualAddress());

    struct RootConstants {
        uint32_t DiffuseIndex;
        uint32_t SpecularIndex;
        uint32_t NormalIndex;
        uint32_t ShadowIndex;
    } constants{};

    constants.ShadowIndex = shadowMapTexture.GetSrvIndex();
    for (const auto& item : renderItems) {
        cmdList->SetGraphicsRootConstantBufferView(2, item.TransformCB->GetGPUVirtualAddress());
        constants.DiffuseIndex = item.Material.DiffuseIndex;
        constants.SpecularIndex = item.Material.SpecularIndex;
        constants.NormalIndex = item.Material.NormalIndex;
        cmdList->SetGraphicsRoot32BitConstants(3, 4, &constants, 0);
        item.Mesh->Draw(commandList);
    }
}

} // namespace GEngine::RenderPass
