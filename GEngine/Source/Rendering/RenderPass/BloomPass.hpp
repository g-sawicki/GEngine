#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Interop/Bloom.h"

#include <array>

namespace GEngine::RenderPass {

struct BloomSettings {
    float Threshold{1.0f};
    float Intensity{1.0f};
    uint32_t WeightCount{4};
    std::array<float, kMaxBloomOffset> Weights{0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
    uint64_t WeightsRevision{}; // Increment after changing Weights so each frame's GPU buffer is refreshed.
};

class BloomPass {
  public:
    explicit BloomPass(Device& device);

    GE_NO_COPY_NO_MOVE(BloomPass)

    void UploadWeights(Buffer& weightsBuffer, const BloomSettings& settings) const;

    void Dispatch(CommandList& commandList, uint32_t inputSrvIndex, uint32_t outputUavIndex, bool horizontal,
                  const Buffer& sceneInfoBuffer, uint32_t width, uint32_t height, const Buffer& weightsBuffer,
                  const BloomSettings& settings);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
