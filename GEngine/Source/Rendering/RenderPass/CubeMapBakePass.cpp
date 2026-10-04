#include "PCH.hpp"

#include "CubeMapBakePass.hpp"

#include "Graphics/D3D12/Shader.hpp"

namespace GEngine::RenderPass {

CubeMapBakePass::CubeMapBakePass(Device& device, DXGI_FORMAT outputFormat)
    : m_RootSignature(CreateRootSignature(device)),
      m_EquirectangularToCubePipeline(
          CreatePipelineState(device, m_RootSignature, "Assets/Shaders/equirectangular_to_cubemap_vs.cso",
                              "Assets/Shaders/equirectangular_to_cubemap_ps.cso", outputFormat)),
      m_IrradianceConvolutionPipeline(CreatePipelineState(device, m_RootSignature,
                                                          "Assets/Shaders/cubemap_convolution_vs.cso",
                                                          "Assets/Shaders/cubemap_convolution_ps.cso", outputFormat)) {}

RootSignature CubeMapBakePass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsConstantBufferView(0);
    rootParams[1].InitAsConstants(2, 1);

    D3D12_STATIC_SAMPLER_DESC staticSampler{
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
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
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return RootSignature(device, rootSigDesc);
}

PipelineState CubeMapBakePass::CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                                   const std::filesystem::path& vertexShaderPath,
                                                   const std::filesystem::path& pixelShaderPath,
                                                   DXGI_FORMAT outputFormat) {
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    const Shader vertexShader{vertexShaderPath};
    const Shader pixelShader{pixelShaderPath};

    D3D12_RASTERIZER_DESC rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    D3D12_DEPTH_STENCIL_DESC depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depthStencilState.DepthEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .VS = vertexShader.GetBytecode(),
        .PS = pixelShader.GetBytecode(),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = rasterizerState,
        .DepthStencilState = depthStencilState,
        .InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {outputFormat},
        .DSVFormat = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0},
    };

    return PipelineState(device, psoDesc);
}

void CubeMapBakePass::EquirectangularToCube(CommandList& commandList, const Texture& cubeMapTexture,
                                            uint32_t panoramaSrvIndex, Buffer& cubeFaceCameraBuffer,
                                            const RenderItem& cubeRenderItem) {
    RenderCubeFaces(commandList, m_EquirectangularToCubePipeline, cubeMapTexture, panoramaSrvIndex,
                    cubeFaceCameraBuffer, cubeRenderItem);
}

void CubeMapBakePass::ConvolveIrradiance(CommandList& commandList, const Texture& irradianceTexture,
                                         uint32_t cubeMapSrvIndex, Buffer& cubeFaceCameraBuffer,
                                         const RenderItem& cubeRenderItem) {
    RenderCubeFaces(commandList, m_IrradianceConvolutionPipeline, irradianceTexture, cubeMapSrvIndex,
                    cubeFaceCameraBuffer, cubeRenderItem);
}

void CubeMapBakePass::RenderCubeFaces(CommandList& commandList, PipelineState& pipelineState,
                                      const Texture& targetTexture, uint32_t inputSrvIndex,
                                      const Buffer& cubeFaceCameraBuffer, const RenderItem& cubeRenderItem) {
    auto* cmdList = commandList.GetHandle();

    const D3D12_VIEWPORT viewport{0.0f,
                                  0.0f,
                                  static_cast<float>(targetTexture.GetDesc().Width),
                                  static_cast<float>(targetTexture.GetDesc().Height),
                                  0.0f,
                                  1.0f};
    const D3D12_RECT scissorRect{0, 0, static_cast<LONG>(targetTexture.GetDesc().Width),
                                 static_cast<LONG>(targetTexture.GetDesc().Height)};
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(pipelineState.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, cubeFaceCameraBuffer.GetGPUVirtualAddress());

    struct RootConstants {
        uint32_t InputIndex;
        uint32_t CameraIndex;
    };

    for (uint16_t i{}; i < 6; ++i) {
        const D3D12_CPU_DESCRIPTOR_HANDLE colorRtv = targetTexture.GetRtvHandle(i);
        cmdList->OMSetRenderTargets(1, &colorRtv, FALSE, nullptr);

        RootConstants constants{.InputIndex = inputSrvIndex, .CameraIndex = i};
        cmdList->SetGraphicsRoot32BitConstants(1, 2, &constants, 0);

        auto vbv{cubeRenderItem.Mesh->VertexBuffer.GetVBV(cubeRenderItem.Mesh->VertexStride)};
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        auto ibv{cubeRenderItem.Mesh->IndexBuffer.GetIBV()};
        cmdList->IASetIndexBuffer(&ibv);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawIndexedInstanced(cubeRenderItem.Mesh->IndexCount, 1, 0, 0, 0);
    }
}

} // namespace GEngine::RenderPass
