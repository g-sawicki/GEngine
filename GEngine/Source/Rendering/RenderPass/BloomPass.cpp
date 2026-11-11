#include "PCH.hpp"

#include "BloomPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Shader.hpp"
#include "Interop/Bloom.h"

#include <algorithm>

namespace GEngine::RenderPass {

BloomPass::BloomPass(Device& device)
    : m_RootSignature(CreateRootSignature(device)), m_PipelineState(CreatePipelineState(device, m_RootSignature)) {}

RootSignature BloomPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstants(RootConstantCount<BloomConstants>, 0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return RootSignature(device, rootSigDesc);
}

PipelineState BloomPass::CreatePipelineState(Device& device, const RootSignature& rootSignature) {
    const Shader computeShader{"Assets/Shaders/bloom_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .CS = computeShader.GetBytecode(),
    };

    return PipelineState(device, psoDesc);
}

void BloomPass::UploadWeights(Buffer& weightsBuffer, const BloomSettings& settings) const {
    BloomWeights weights{};
    for (uint32_t i = 0; i < kMaxBloomOffset; i += 4) {
        DirectX::XMFLOAT4& packedWeights = weights.Weights[i / 4];
        packedWeights.x = settings.Weights[i];
        packedWeights.y = settings.Weights[i + 1];
        packedWeights.z = settings.Weights[i + 2];
        packedWeights.w = settings.Weights[i + 3];
    }

    auto* weightsData = static_cast<BloomWeights*>(weightsBuffer.Map());
    *weightsData = weights;
    weightsBuffer.Unmap();
}

void BloomPass::Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex, bool horizontal,
                         const Buffer& sceneInfoBuffer, uint32_t width, uint32_t height, const Buffer& weightsBuffer,
                         const BloomSettings& settings) {
    auto* cmdList = commandList.GetHandle();
    cmdList->SetComputeRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());

    BloomConstants constants{
        .InputIndex = inputSrvIndex,
        .OutputIndex = outputUavIndex,
        .Horizontal = horizontal ? 1u : 0u,
        .WeightCount = std::min(settings.WeightCount, kMaxBloomOffset - 1),
        .Threshold = settings.Threshold,
        .Intensity = settings.Intensity,
    };

    cmdList->SetComputeRoot32BitConstants(0, RootConstantCount<BloomConstants>, &constants, 0);
    cmdList->SetComputeRootConstantBufferView(1, sceneInfoBuffer.GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(2, weightsBuffer.GetGPUVirtualAddress());

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
