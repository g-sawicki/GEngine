#include "PCH.hpp"

#include "SSAOPass.hpp"

#include "Core/Utility/Math.hpp"
#include "Core/Utility/MersenneTwister.hpp"
#include "Graphics/D3D12/Shader.hpp"
#include "Interop/SSAO.h"
#include "Rendering/UploadEngine.hpp"

#include <cmath>

namespace GEngine::RenderPass {

SSAOPass::SSAOPass(Device& device, UploadEngine& uploadEngine)
    : m_RootSignature(CreateRootSignature(device)), m_PipelineState(CreatePipelineState(device, m_RootSignature)),
      m_SampleOffsetsBuffer(CreateSampleOffsetsBuffer(device)),
      m_RandomVectorTexture(CreateRandomVectorTexture(device, uploadEngine)) {}

RootSignature SSAOPass::CreateRootSignature(Device& device) {
    CD3DX12_ROOT_PARAMETER1 rootParams[3]{};
    rootParams[0].InitAsConstants(7, 0);
    rootParams[1].InitAsConstantBufferView(1);
    rootParams[2].InitAsConstantBufferView(2);

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
    staticSamplers[0] = D3D12_STATIC_SAMPLER_DESC{
        .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .ShaderRegister = 0,
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

PipelineState SSAOPass::CreatePipelineState(Device& device, const RootSignature& rootSignature) {
    const Shader computeShader{"Assets/Shaders/ssao_cs.cso"};

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = rootSignature.Get(),
        .CS = computeShader.GetBytecode(),
    };

    return PipelineState(device, psoDesc);
}

Buffer SSAOPass::CreateSampleOffsetsBuffer(Device& device) {
    std::vector<DirectX::XMFLOAT4> offsets;
    offsets.reserve(kMaxSamples);

    using RNG = GEngine::MersenneTwister;

    while (offsets.size() < static_cast<size_t>(kMaxSamples)) {
        const float x = RNG::GetRandom(-1.0f, 1.0f);
        const float y = RNG::GetRandom(-1.0f, 1.0f);
        const float z = RNG::GetRandom(0.0f, 1.0f);
        const float distanceSquared = x * x + y * y + z * z;

        if (distanceSquared > 1.0f || distanceSquared <= 0.0f)
            continue;

        offsets.emplace_back(x, y, z, 0.0f);
    }

    Buffer buffer{device, BufferDesc{.Size = sizeof(SampleData),
                                     .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                     .MiscFlags = BufferMiscFlags::ConstantBuffer}};

    auto* data = static_cast<SampleData*>(buffer.Map());
    std::copy(offsets.begin(), offsets.end(), std::begin(data->SampleOffsets));
    buffer.Unmap();

    return buffer;
}

Texture SSAOPass::CreateRandomVectorTexture(Device& device, UploadEngine& uploadEngine) {
    constexpr size_t kTexelCount = kRandomVectorTextureSize * kRandomVectorTextureSize;
    std::array<DirectX::XMFLOAT4, kTexelCount> randomVectors;

    using RNG = GEngine::MersenneTwister;
    for (DirectX::XMFLOAT4& vector : randomVectors) {
        const float angle = RNG::GetRandom(0.0f, DirectX::XM_2PI);
        vector = {std::cos(angle), std::sin(angle), 0.0f, 0.0f};
    }

    Texture texture{device, TextureDesc{
                                .Width = kRandomVectorTextureSize,
                                .Height = kRandomVectorTextureSize,
                                .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                                .Usage = TextureUsage::ShaderResource,
                            }};
    const SubresourceData data{randomVectors.data(), sizeof(DirectX::XMFLOAT4) * kRandomVectorTextureSize};
    uploadEngine.Begin();
    uploadEngine.UploadTexture(texture, {&data, 1});
    uploadEngine.Submit();
    return texture;
}

void SSAOPass::Dispatch(CommandList& commandList, uint32_t normalIndex, uint32_t depthIndex,
                        uint32_t ambientOcclusionIndex, const Buffer& sceneInfoCB, uint32_t width, uint32_t height,
                        const SSAOSettings& settings) {
    auto* cmdList = commandList.GetHandle();

    SSAOConstants constants{
        .NormalIndex = normalIndex,
        .DepthIndex = depthIndex,
        .RandomVectorIndex = m_RandomVectorTexture.GetSrvIndex(),
        .AmbientOcclusionIndex = ambientOcclusionIndex,
        .SampleCount = std::clamp(settings.SampleCount, 1u, kMaxSamples),
        .SampleRadius = settings.SampleRadius,
        .DepthBias = settings.DepthBias,
    };

    const UINT groupCountX = DivideRoundUp(width, 8u);
    const UINT groupCountY = DivideRoundUp(height, 8u);

    cmdList->SetComputeRootSignature(m_RootSignature.Get());
    cmdList->SetPipelineState(m_PipelineState.Get());
    cmdList->SetComputeRoot32BitConstants(0, 7, &constants, 0);
    cmdList->SetComputeRootConstantBufferView(1, sceneInfoCB.GetGPUVirtualAddress());
    cmdList->SetComputeRootConstantBufferView(2, m_SampleOffsetsBuffer.GetGPUVirtualAddress());
    cmdList->Dispatch(groupCountX, groupCountY, 1);
}

} // namespace GEngine::RenderPass
