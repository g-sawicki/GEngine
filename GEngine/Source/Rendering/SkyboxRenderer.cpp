#include "PCH.hpp"

#include "Rendering/SkyboxRenderer.hpp"

#include "Core/Utility/Image.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Rendering/GpuResourceCache.hpp"
#include "Rendering/MeshFactory.hpp"
#include "Rendering/UploadEngine.hpp"

#include <cstring>

namespace GEngine {

namespace {

constexpr DirectX::XMFLOAT3 kCubeMapFaceDirections[6] = {
    {1.0f, 0.0f, 0.0f},  {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f, -1.0f},
};

constexpr DirectX::XMFLOAT3 kCubeMapFaceUpVectors[6] = {
    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
};

constexpr DXGI_FORMAT kSkyboxCubeMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
constexpr uint32_t kIrradianceSize = 32;

[[nodiscard]] RenderPass::CubeFaceCameraData BuildCubeFaceCameras() {
    RenderPass::CubeFaceCameraData cameraData{};

    const float fovY = DirectX::XMConvertToRadians(90.0f);
    for (uint32_t i{}; i < std::size(kCubeMapFaceDirections); ++i) {
        const DirectX::XMMATRIX view =
            DirectX::XMMatrixLookToLH(DirectX::XMVectorZero(), DirectX::XMLoadFloat3(&kCubeMapFaceDirections[i]),
                                      DirectX::XMLoadFloat3(&kCubeMapFaceUpVectors[i]));
        const DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(fovY, 1.0f, 0.01f, 100.0f);
        DirectX::XMStoreFloat4x4(&cameraData.ViewProjection[i], DirectX::XMMatrixMultiply(view, projection));
    }

    return cameraData;
}

void TransitionResource(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* resource,
                        const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after) {
    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after)};
    cmdList->ResourceBarrier(1, &barrier);
}

} // namespace

SkyboxRenderer::SkyboxRenderer(Device& device, UploadEngine& uploadEngine, GpuResourceCache& resources,
                               const DXGI_FORMAT colorFormat, const DXGI_FORMAT depthFormat)
    : m_Device(device), m_UploadEngine(uploadEngine), m_Resources(resources),
      m_CubeMapBakePass(device, kSkyboxCubeMapFormat), m_SkyboxPass(device, colorFormat, depthFormat),
      m_CubeFaceCameraCB(device, BufferDesc{.Size = sizeof(RenderPass::CubeFaceCameraData),
                                            .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                            .MiscFlags = BufferMiscFlags::ConstantBuffer}) {
    m_UploadEngine.Begin();
    m_CubeMesh = m_Resources.StageMesh(MeshFactory::Cube());
    m_UploadEngine.Submit();

    const RenderPass::CubeFaceCameraData cameraData = BuildCubeFaceCameras();
    void* const dst = m_CubeFaceCameraCB.Map();
    std::memcpy(dst, &cameraData, sizeof(cameraData));
    m_CubeFaceCameraCB.Unmap();
}

void SkyboxRenderer::Update(const Skybox& skybox) {
    if (m_Path == skybox.Path)
        return;

    Image panorama{skybox.Path};

    m_UploadEngine.Begin();

    TextureDesc panoramaDesc{.Width = panorama.GetWidth(),
                             .Height = panorama.GetHeight(),
                             .Format = panorama.GetFormat(),
                             .Usage = TextureUsage::ShaderResource};
    m_PanoramaTexture = std::make_unique<Texture>();
    m_PanoramaTexture->Create(m_Device, panoramaDesc);
    const SubresourceData data{panorama.GetData().data()};
    m_UploadEngine.UploadTexture(*m_PanoramaTexture, {&data, 1});

    m_Path = skybox.Path;

    const uint32_t cubeMapSize = panorama.GetHeight();
    const TextureDesc cubeMapDesc{
        .Width = cubeMapSize,
        .Height = cubeMapSize,
        .DepthOrArraySize = 6,
        .Format = kSkyboxCubeMapFormat,
        .Usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget,
        .IsCubeMap = true,
        .ClearValue = {.Format = kSkyboxCubeMapFormat, .Color = {0.0f, 0.0f, 0.0f, 1.0f}},
        .InitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    };
    m_CubeMapTexture = std::make_unique<Texture>(m_Device, cubeMapDesc);

    const TextureDesc irradianceDesc{
        .Width = kIrradianceSize,
        .Height = kIrradianceSize,
        .DepthOrArraySize = 6,
        .Format = kSkyboxCubeMapFormat,
        .Usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget,
        .IsCubeMap = true,
        .ClearValue = {.Format = kSkyboxCubeMapFormat, .Color = {0.0f, 0.0f, 0.0f, 1.0f}},
        .InitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    };
    m_IrradianceTexture = std::make_unique<Texture>(m_Device, irradianceDesc);

    m_NeedsBake = true;

    m_UploadEngine.Submit();
}

void SkyboxRenderer::BakeEnvironmentMaps(CommandList& commandList) {
    if (!m_NeedsBake)
        return;

    auto* cmdList = commandList.GetHandle();
    const RenderItem cubeRenderItem{.Mesh = &m_CubeMesh};

    // Equirectangular panorama -> cube map
    TransitionResource(cmdList, m_CubeMapTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                       D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_CubeMapBakePass.EquirectangularToCube(commandList, *m_CubeMapTexture, m_PanoramaTexture->GetSrvIndex(),
                                            m_CubeFaceCameraCB, cubeRenderItem);
    TransitionResource(cmdList, m_CubeMapTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Cube map -> diffuse irradiance
    TransitionResource(cmdList, m_IrradianceTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                       D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_CubeMapBakePass.ConvolveIrradiance(commandList, *m_IrradianceTexture, m_CubeMapTexture->GetSrvIndex(),
                                         m_CubeFaceCameraCB, cubeRenderItem);
    TransitionResource(cmdList, m_IrradianceTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_NeedsBake = false;
}

uint32_t SkyboxRenderer::GetIrradianceSrvIndex() const noexcept {
    return m_IrradianceTexture ? m_IrradianceTexture->GetSrvIndex() : INVALID_BINDLESS_INDEX;
}

void SkyboxRenderer::Render(CommandList& commandList, const Texture& colorTarget, const Texture& depthTarget,
                            Buffer& sceneInfoCB) {
    if (!m_CubeMapTexture || m_CubeMapTexture->GetSrvIndex() == INVALID_BINDLESS_INDEX)
        return;

    m_SkyboxPass.OnRender(commandList, m_CubeMesh, colorTarget, depthTarget, m_CubeMapTexture->GetSrvIndex(),
                          sceneInfoCB);
}

} // namespace GEngine
