#include "PCH.hpp"

#include "ToneMapPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Shader.hpp"

namespace GEngine::RenderPass {

ToneMapPass::ToneMapPass(Device& device)
    : m_RootSignature(CreateRootSignature(device)), m_PipelineState(CreatePipelineState(device, m_RootSignature)) {}

RootSignature ToneMapPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsConstants(RootConstantCount<ToneMapConstants>, 0);
    rootParams[1].InitAsConstantBufferView(1);

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return RootSignature(device, rootSigDesc);
}

PipelineState ToneMapPass::CreatePipelineState(Device& device, const RootSignature& rootSignature) {
    const Shader computeShader{"Assets/Shaders/tonemap_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .CS = computeShader.GetBytecode(),
    };

    return PipelineState(device, psoDesc);
}

void ToneMapPass::Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex,
                           Buffer& sceneInfoBuffer, uint32_t width, uint32_t height, const ToneMapSettings& settings) {
    auto* cmdList = commandList.GetHandle();
    cmdList->SetComputeRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());

    ToneMapConstants constants{
        .InputIndex = inputSrvIndex,
        .OutputIndex = outputUavIndex,
        .TonemapMode = settings.Mode,
        .MaxWhite = settings.MaxWhite,
    };

    cmdList->SetComputeRoot32BitConstants(0, RootConstantCount<ToneMapConstants>, &constants, 0);
    cmdList->SetComputeRootConstantBufferView(1, sceneInfoBuffer.GetGPUVirtualAddress());

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
