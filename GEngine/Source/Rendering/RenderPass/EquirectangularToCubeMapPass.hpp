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

namespace GEngine::RenderPass {

struct EquirectangularToCubeMapCameraData {
    DirectX::XMFLOAT4X4 ViewProjection[6];
};

class EquirectangularToCubeMapPass {
  public:
    explicit EquirectangularToCubeMapPass(Device& device, const Texture& outputTexture);

    GE_NO_COPY_NO_MOVE(EquirectangularToCubeMapPass)

    void OnRender(CommandList& commandList, const Texture& outputTexture, uint32_t inputSrvIndex,
                  Buffer& cameraDataBuffer, const RenderItem& renderItem);

    [[nodiscard]] DXGI_FORMAT GetOutputFormat() const noexcept { return m_OutputFormat; }

  private:
    std::unique_ptr<RootSignature> m_RootSignature;
    std::unique_ptr<PipelineState> m_PipelineState;

    DXGI_FORMAT m_OutputFormat{};
};

} // namespace GEngine::RenderPass
