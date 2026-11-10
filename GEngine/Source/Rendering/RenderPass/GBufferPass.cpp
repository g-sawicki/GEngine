#include "PCH.hpp"

#include "GBufferPass.hpp"

#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Shader.hpp"
#include "Interop/GBuffer.h"

namespace GEngine::RenderPass {

GBufferPass::GBufferPass(Device& device, const GBufferFormats& gBufferFormats, DXGI_FORMAT depthFormat)
    : m_RootSignature(CreateRootSignature(device)),
      m_PipelineState(CreatePipelineState(device, m_RootSignature, gBufferFormats, depthFormat)) {}

RootSignature GBufferPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstants(RootConstantCount<GBufferConstants>, 0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);

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

    return RootSignature(device, rootSigDesc);
}

PipelineState GBufferPass::CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                               const GBufferFormats& gBufferFormats, const DXGI_FORMAT depthFormat) {
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    const Shader vertexShader{"Assets/Shaders/gbuffer_vs.cso"};
    const Shader pixelShader{"Assets/Shaders/gbuffer_ps.cso"};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .VS = vertexShader.GetBytecode(),
        .PS = pixelShader.GetBytecode(),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 3,
        .RTVFormats = {gBufferFormats.AlbedoFormat, gBufferFormats.NormalFormat,
                       gBufferFormats.RoughnessMetallicFormat},
        .DSVFormat = depthFormat,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    return PipelineState(device, psoDesc);
}

void GBufferPass::OnRender(CommandList& commandList, const GBufferTextures& gBufferTextures,
                           const Texture& depthTexture, const Buffer& sceneInfoCB,
                           std::span<const RenderItem> renderItems) {
    auto* cmdList = commandList.GetHandle();

    cmdList->ClearRenderTargetView(gBufferTextures.AlbedoTexture.GetRtvHandle(),
                                   gBufferTextures.AlbedoTexture.GetDesc().ClearValue.Color, 0, nullptr);
    cmdList->ClearRenderTargetView(gBufferTextures.NormalTexture.GetRtvHandle(),
                                   gBufferTextures.NormalTexture.GetDesc().ClearValue.Color, 0, nullptr);
    cmdList->ClearRenderTargetView(gBufferTextures.RoughnessMetallicTexture.GetRtvHandle(),
                                   gBufferTextures.RoughnessMetallicTexture.GetDesc().ClearValue.Color, 0, nullptr);
    cmdList->ClearDepthStencilView(depthTexture.GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{0.0f,
                            0.0f,
                            static_cast<float>(gBufferTextures.AlbedoTexture.GetDesc().Width),
                            static_cast<float>(gBufferTextures.AlbedoTexture.GetDesc().Height),
                            0.0f,
                            1.0f};
    D3D12_RECT scissorRect{0, 0, static_cast<LONG>(gBufferTextures.AlbedoTexture.GetDesc().Width),
                           static_cast<LONG>(gBufferTextures.AlbedoTexture.GetDesc().Height)};
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 3> renderTargetViews{
        gBufferTextures.AlbedoTexture.GetRtvHandle(),
        gBufferTextures.NormalTexture.GetRtvHandle(),
        gBufferTextures.RoughnessMetallicTexture.GetRtvHandle(),
    };
    const D3D12_CPU_DESCRIPTOR_HANDLE depthDsv = depthTexture.GetDsvHandle();
    cmdList->OMSetRenderTargets(static_cast<UINT>(renderTargetViews.size()), renderTargetViews.data(), FALSE,
                                &depthDsv);

    cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());
    cmdList->SetGraphicsRootConstantBufferView(1, sceneInfoCB.GetGPUVirtualAddress());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for (const auto& item : renderItems) {
        if (!item.CameraVisible)
            continue;

        const GBufferConstants constants{
            .AlbedoIndex = item.Material.AlbedoIndex,
            .NormalIndex = item.Material.NormalIndex,
            .RoughnessMetallicIndex = item.Material.RoughnessMetallicIndex,
        };
        cmdList->SetGraphicsRoot32BitConstants(0, RootConstantCount<GBufferConstants>, &constants, 0);
        cmdList->SetGraphicsRootConstantBufferView(2, item.ObjectConstantBuffer);

        auto vbv{item.Mesh->VertexBuffer.GetVBV(item.Mesh->VertexStride)};
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        auto ibv{item.Mesh->IndexBuffer.GetIBV()};
        cmdList->IASetIndexBuffer(&ibv);

        cmdList->DrawIndexedInstanced(item.Mesh->IndexCount, 1, 0, 0, 0);
    }
}

} // namespace GEngine::RenderPass
