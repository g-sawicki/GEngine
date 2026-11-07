#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"

namespace GEngine {
class UploadEngine;
}

namespace GEngine::RenderPass {

struct SSAOSettings {
    float SampleRadius{0.1f};
};

class SSAOPass {
  public:
    SSAOPass(Device& device, UploadEngine& uploadEngine);

    GE_NO_COPY_NO_MOVE(SSAOPass)

    void Dispatch(CommandList& commandList, uint32_t normalIndex, uint32_t depthIndex,
                  const Texture& ambientOcclusionTexture, const Buffer& sceneInfoCB, const SSAOSettings& settings);

  private:
    static Buffer CreateSampleOffsetsBuffer(Device& device);
    static Texture CreateRandomVectorTexture(Device& device, UploadEngine& uploadEngine);

    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;

    Buffer m_SampleOffsetsBuffer;
    Texture m_RandomVectorTexture;
};

} // namespace GEngine::RenderPass
