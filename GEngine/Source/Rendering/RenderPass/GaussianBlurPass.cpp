#include "PCH.hpp"

#include "GaussianBlurPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Shader.hpp"
#include "Interop/GaussianBlur.h"

#include <algorithm>

namespace GEngine::RenderPass {

GaussianBlurPass::GaussianBlurPass(Device& device)
    : m_RootSignature(CreateRootSignature(device)), m_PipelineState(CreatePipelineState(device, m_RootSignature)) {}

RootSignature GaussianBlurPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstants(RootConstantCount<GaussianBlurConstants>, 0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return RootSignature(device, rootSigDesc);
}

PipelineState GaussianBlurPass::CreatePipelineState(Device& device, const RootSignature& rootSignature) {
    const Shader computeShader{"Assets/Shaders/gaussian_blur_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .CS = computeShader.GetBytecode(),
    };

    return PipelineState(device, psoDesc);
}

void GaussianBlurPass::UploadWeights(Buffer& weightsBuffer, const GaussianBlurSettings& settings) const {
    GaussianWeights weights{};
    for (uint32_t i = 0; i < kMaxOffset; i += 4) {
        DirectX::XMFLOAT4& packedWeights = weights.Weights[i / 4];
        packedWeights.x = settings.Weights[i];
        packedWeights.y = settings.Weights[i + 1];
        packedWeights.z = settings.Weights[i + 2];
        packedWeights.w = settings.Weights[i + 3];
    }
    auto* weightsData = static_cast<GaussianWeights*>(weightsBuffer.Map());
    *weightsData = weights;
    weightsBuffer.Unmap();
}

void GaussianBlurPass::Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex,
                                bool horizontal, const Buffer& sceneInfoBuffer, uint32_t width, uint32_t height,
                                const Buffer& weightsBuffer, const GaussianBlurSettings& settings) {
    auto* cmdList = commandList.GetHandle();

    const GaussianBlurConstants constants{
        .InputIndex = inputSrvIndex,
        .OutputIndex = outputUavIndex,
        .Horizontal = horizontal ? 1u : 0u,
        .WeightCount = std::min(settings.WeightCount, kMaxOffset - 1),
    };

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);

    cmdList->SetComputeRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());
    cmdList->SetComputeRoot32BitConstants(0, RootConstantCount<GaussianBlurConstants>, &constants, 0);
    cmdList->SetComputeRootConstantBufferView(1, sceneInfoBuffer.GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(2, weightsBuffer.GetGPUVirtualAddress());
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
