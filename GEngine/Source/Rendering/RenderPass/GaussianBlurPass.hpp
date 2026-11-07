#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Interop/GaussianBlur.h"

#include <array>

namespace GEngine::RenderPass {

struct GaussianBlurSettings {
    // Radius in texels; weights are indexed from the center and reused symmetrically.
    uint32_t WeightCount{4};
    std::array<float, kMaxOffset> Weights{0.204164f, 0.180174f, 0.123832f, 0.066282f, 0.027631f};

    bool operator==(const GaussianBlurSettings&) const = default;
};

class GaussianBlurPass {
  public:
    explicit GaussianBlurPass(Device& device);

    GE_NO_COPY_NO_MOVE(GaussianBlurPass)

    void UploadWeights(Buffer& weightsBuffer, const GaussianBlurSettings& settings) const;

    void Dispatch(CommandList& commandList, uint32_t inputSrvIndex, const Texture& outputTexture, bool horizontal,
                  const Buffer& sceneInfoBuffer, const Buffer& weightsBuffer, const GaussianBlurSettings& settings);

  private:
    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature);

    RootSignature m_RootSignature;
    PipelineState m_PipelineState;
};

} // namespace GEngine::RenderPass
