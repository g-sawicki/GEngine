#include "PCH.hpp"

#include "DeferredLightingPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Graphics/D3D12/Shader.hpp"
#include "Interop/DeferredLighting.h"

namespace GEngine::RenderPass {

DeferredLightingPass::DeferredLightingPass(Device& device)
    : m_RootSignature(CreateRootSignature(device)), m_PipelineState(CreatePipelineState(device, m_RootSignature)) {}

RootSignature DeferredLightingPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstants(RootConstantCount<DeferredLightingConstants>, 0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[3]{};
    staticSamplers[0] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_ANISOTROPIC,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
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
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
    staticSamplers[2] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 1,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .MinLOD = 0.0f,
        .MaxLOD = 0.0f,
        .ShaderRegister = 2,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };

    D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = static_cast<UINT>(std::size(staticSamplers));
    rootSigDesc.pStaticSamplers = staticSamplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    return RootSignature(device, rootSigDesc);
}

PipelineState DeferredLightingPass::CreatePipelineState(Device& device, const RootSignature& rootSignature) {
    const Shader computeShader{"Assets/Shaders/deferred_lighting_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .CS = computeShader.GetBytecode(),
    };

    return PipelineState(device, psoDesc);
}

void DeferredLightingPass::Dispatch(CommandList& commandList, uint32_t albedoIndex, uint32_t normalIndex,
                                    uint32_t roughnessMetallicIndex, uint32_t ambientOcclusionIndex,
                                    uint32_t depthIndex, uint32_t shadowMapIndex, uint32_t hdrIndex,
                                    const Buffer& sceneInfoCB, const Buffer& cascadedShadowMapsDataCB, uint32_t width,
                                    uint32_t height) {
    auto* cmdList = commandList.GetHandle();

    DeferredLightingConstants constants{
        .AlbedoIndex = albedoIndex,
        .NormalIndex = normalIndex,
        .RoughnessMetallicIndex = roughnessMetallicIndex,
        .AmbientOcclusionIndex = ambientOcclusionIndex,
        .DepthIndex = depthIndex,
        .ShadowIndex = shadowMapIndex,
        .HdrIndex = hdrIndex,
    };

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);

    cmdList->SetComputeRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());
    cmdList->SetComputeRoot32BitConstants(0, RootConstantCount<DeferredLightingConstants>, &constants, 0);
    cmdList->SetComputeRootConstantBufferView(1, sceneInfoCB.GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(2, cascadedShadowMapsDataCB.GetGPUVirtualAddress());
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
