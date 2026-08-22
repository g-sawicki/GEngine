#include "PCH.hpp"

#include "ToneMapPass.hpp"

#include "Core/Utility/Common.hpp"
#include "Graphics/D3D12/Shader.hpp"

namespace GEngine::RenderPass {

ToneMapPass::ToneMapPass(Device& device) {
    CD3DX12_ROOT_PARAMETER rootParams[3]{};
    rootParams[0].InitAsConstantBufferView(0); // b0: SceneInfo

    CD3DX12_DESCRIPTOR_RANGE hdrRange{};
    hdrRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0: HDR input
    CD3DX12_DESCRIPTOR_RANGE outputRange{};
    outputRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0: tonemapped output
    rootParams[1].InitAsDescriptorTable(1, &hdrRange);
    rootParams[2].InitAsDescriptorTable(1, &outputRange);

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

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &staticSampler;

    m_RootSignature = std::make_unique<RootSignature>(device, rootSigDesc);

    const Shader computeShader{"Assets/Shaders/tonemap_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = m_RootSignature->Get(),
        .CS = computeShader.GetBytecode(),
    };

    m_PipelineState = std::make_unique<PipelineState>(device, psoDesc);
}

void ToneMapPass::Dispatch(CommandList& commandList, D3D12_GPU_DESCRIPTOR_HANDLE hdrSRV,
                           D3D12_GPU_DESCRIPTOR_HANDLE outputUAV, Buffer& sceneInfoBuffer, uint32_t width,
                           uint32_t height) {
    auto* cmdList = commandList.GetHandle();
    cmdList->SetComputeRootSignature(m_RootSignature->Get());
    cmdList->SetPipelineState(m_PipelineState->Get());

    cmdList->SetComputeRootConstantBufferView(0, sceneInfoBuffer.GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1, hdrSRV);
    cmdList->SetComputeRootDescriptorTable(2, outputUAV);

    const UINT groupCountX = RoundUp<8>(width);
    const UINT groupCountY = RoundUp<8>(height);
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
