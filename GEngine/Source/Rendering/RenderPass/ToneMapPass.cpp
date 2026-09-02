#include "PCH.hpp"

#include "ToneMapPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/Shader.hpp"

namespace GEngine::RenderPass {

ToneMapPass::ToneMapPass(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: SceneInfo
    rootParams[1].InitAsConstants(2, 1);       // b1: RootConstants

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
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    const Shader computeShader{"Assets/Shaders/tonemap_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = m_RootSignature->Get(),
        .CS = computeShader.GetBytecode(),
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

void ToneMapPass::Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex,
                           Buffer& sceneInfoBuffer, uint32_t width, uint32_t height) {
    auto* cmdList = commandList.GetHandle();
    cmdList->SetComputeRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());

    struct RootConstants {
        uint32_t InputIndex;
        uint32_t OutputIndex;
    } constants{.InputIndex = inputSrvIndex, .OutputIndex = outputUavIndex};

    cmdList->SetComputeRootConstantBufferView(0, sceneInfoBuffer.GetGPUVirtualAddress());
    cmdList->SetComputeRoot32BitConstants(1, 2, &constants, 0);

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
