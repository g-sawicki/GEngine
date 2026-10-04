#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/PipelineState.hpp"
#include "Graphics/D3D12/RootSignature.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"

#include <DirectXMath.h>
#include <filesystem>

namespace GEngine::RenderPass {

struct CubeFaceCameraData {
    DirectX::XMFLOAT4X4 ViewProjection[6];
};

class CubeMapBakePass {
  public:
    CubeMapBakePass(Device& device, DXGI_FORMAT outputFormat);

    GE_NO_COPY_NO_MOVE(CubeMapBakePass)

    void EquirectangularToCube(CommandList& commandList, const Texture& cubeMapTexture, uint32_t panoramaSrvIndex,
                               Buffer& cubeFaceCameraBuffer, const RenderItem& cubeRenderItem);

    void ConvolveIrradiance(CommandList& commandList, const Texture& irradianceTexture, uint32_t cubeMapSrvIndex,
                            Buffer& cubeFaceCameraBuffer, const RenderItem& cubeRenderItem);

  private:
    void RenderCubeFaces(CommandList& commandList, PipelineState& pipelineState, const Texture& targetTexture,
                         uint32_t inputSrvIndex, const Buffer& cubeFaceCameraBuffer, const RenderItem& cubeRenderItem);

    static RootSignature CreateRootSignature(Device& device);
    static PipelineState CreatePipelineState(Device& device, const RootSignature& rootSignature,
                                             const std::filesystem::path& vertexShaderPath,
                                             const std::filesystem::path& pixelShaderPath, DXGI_FORMAT outputFormat);

    RootSignature m_RootSignature;
    PipelineState m_EquirectangularToCubePipeline;
    PipelineState m_IrradianceConvolutionPipeline;
};

} // namespace GEngine::RenderPass
