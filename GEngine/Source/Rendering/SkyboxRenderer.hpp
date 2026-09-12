#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Rendering/Components.hpp"
#include "Rendering/RenderPass/EquirectangularToCubeMapPass.hpp"
#include "Rendering/RenderPass/SkyboxPass.hpp"
#include "Scene/Skybox.hpp"

#include <filesystem>
#include <memory>

namespace GEngine {

class Device;
class GpuResourceCache;
class UploadEngine;

class SkyboxRenderer {
  public:
    SkyboxRenderer(Device& device, UploadEngine& uploadEngine, GpuResourceCache& resources, const Texture& colorTarget,
                   const Texture& depthTarget);

    GE_NO_COPY_NO_MOVE(SkyboxRenderer)

    void Update(const Skybox& skybox);
    void Render(CommandList& commandList, const Texture& colorTarget, const Texture& depthTarget, Buffer& sceneInfoCB);

  private:
    Device& m_Device;
    UploadEngine& m_UploadEngine;
    GpuResourceCache& m_Resources;

    RenderPass::EquirectangularToCubeMapPass m_EquirectangularToCubeMapPass;
    RenderPass::SkyboxPass m_SkyboxPass;

    Buffer m_EquirectangularToCubeMapCameraCB;
    MeshGPU m_CubeMesh;

    std::unique_ptr<Texture> m_PanoramaTexture;
    std::unique_ptr<Texture> m_CubeMapTexture;
    std::filesystem::path m_Path{};
    bool m_NeedsBake{false};
};

} // namespace GEngine
