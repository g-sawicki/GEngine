#include "PCH.hpp"

#include "Graphics/Renderer.hpp"

#include "Core/Utility/Common.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Rendering/MeshFactory.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

void Renderer::Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp, uint32_t shadowMapSize) {
    Device::EnableDebugLayer();
    ComPtr<IDXGIFactory6> dxgiFactory = Device::CreateDXGIFactory();
    ComPtr<IDXGIAdapter4> dxgiAdapter4{Device::GetAdapter(dxgiFactory.Get(), useWarp)};
    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        std::make_unique<SwapChain>(dxgiFactory.Get(), hwnd, *m_CommandQueue, width, height, SwapChain::NumFrames);

    TextureDesc shadowMapDesc{.Width = shadowMapSize,
                              .Height = shadowMapSize,
                              .Depth = kMaxCascades,
                              .Format = DXGI_FORMAT_D32_FLOAT,
                              .Usage = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
                              .ClearValue = {
                                  .Format = DXGI_FORMAT_D32_FLOAT,
                                  .DepthStencil = {.Depth = 1.0f, .Stencil = 0},
                              }};
    m_ShadowMapTexture.Create(*m_Device, shadowMapDesc);

    CreateRenderTargets(width, height);

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ThrowIfFailed(m_Device->Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              IID_PPV_ARGS(&m_FrameResources[i].CommandAllocator)));
        m_FrameResources[i].CommandList = std::make_unique<CommandList>(
            *m_Device, m_FrameResources[i].CommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        const BufferDesc sceneInfoCbDesc{.Size = sizeof(SceneInfo),
                                         .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                         .MiscFlags = BufferMiscFlags::ConstantBuffer};
        m_FrameResources[i].SceneInfoConstantBuffer =
            std::make_unique<Buffer>(*m_Device, *m_CommandQueue, sceneInfoCbDesc);

        const BufferDesc cascadedShadowMapsDataCbDesc{.Size = sizeof(CascadedShadowMapsData),
                                                      .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                                      .MiscFlags = BufferMiscFlags::ConstantBuffer};
        m_FrameResources[i].CascadedShadowMapsDataConstantBuffer =
            std::make_unique<Buffer>(*m_Device, *m_CommandQueue, cascadedShadowMapsDataCbDesc);

        const BufferDesc LightDataSbDesc{.Size = kMaxLights * sizeof(LightData), .HeapType = D3D12_HEAP_TYPE_UPLOAD};
        m_FrameResources[i].LightDataStructuredBuffer =
            std::make_unique<Buffer>(*m_Device, *m_CommandQueue, LightDataSbDesc);
        m_FrameResources[i].LightDataStructuredBuffer->CreateStructuredBufferSRV(*m_Device, kMaxLights,
                                                                                 sizeof(LightData));
    }

    m_Fence = std::make_unique<Fence>(*m_Device);

    m_ShadowPass = std::make_unique<RenderPass::ShadowPass>(*m_Device, m_ShadowMapTexture.GetDesc().Format);
    m_ForwardLighting = std::make_unique<RenderPass::ForwardLightingPass>(*m_Device, m_HdrTexture, m_DepthTexture);
    m_ToneMapPass = std::make_unique<RenderPass::ToneMapPass>(*m_Device);
    m_SkyboxPass = std::make_unique<RenderPass::SkyboxPass>(*m_Device, m_HdrTexture, m_DepthTexture);
    m_SkyboxMeshBuffer = CreateMeshBuffer(MeshFactory::Cube());
}

void Renderer::CreateRenderTargets(uint32_t width, uint32_t height) {
    TextureDesc depthStencilDesc{.Width = width,
                                 .Height = height,
                                 .Format = DXGI_FORMAT_D32_FLOAT,
                                 .Usage = TextureUsage::DepthStencil,
                                 .ClearValue = {
                                     .Format = DXGI_FORMAT_D32_FLOAT,
                                     .DepthStencil = {.Depth = 1.0f, .Stencil = 0},
                                 }};
    m_DepthTexture.Create(*m_Device, depthStencilDesc);

    TextureDesc hdrDesc{.Width = width,
                        .Height = height,
                        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
                        .Usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget,
                        .ClearValue = {
                            .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
                            .Color = {0.4f, 0.6f, 0.9f, 1.0f},
                        }};
    m_HdrTexture.Create(*m_Device, hdrDesc);

    TextureDesc presentTargetDesc{.Width = width,
                                  .Height = height,
                                  .Format = SwapChain::BackBufferFormat,
                                  .Usage = TextureUsage::UnorderedAccess};
    m_PresentTarget.Create(*m_Device, presentTargetDesc);
}

void Renderer::Destroy() {
    if (m_Fence && m_CommandQueue) {
        m_Fence->Flush(m_CommandQueue->GetHandle());
    }

    // Reset in reverse order of creation.
    for (auto& frame : m_FrameResources) {
        frame.CommandList.reset();
        frame.CommandAllocator.Reset();
        frame.SceneInfoConstantBuffer.reset();
        frame.CascadedShadowMapsDataConstantBuffer.reset();
        frame.LightDataStructuredBuffer.reset();
        frame.ObjectConstantBuffers.clear();
    }
    m_Fence.reset();
    m_SwapChain.reset();
    m_CommandQueue.reset();
    m_ToneMapPass.reset();
    m_ForwardLighting.reset();
    m_ShadowPass.reset();
    m_SkyboxPass.reset();
    m_SkyboxMeshBuffer.reset();

    m_PresentTarget.Reset();
    m_HdrTexture.Reset();
    m_ShadowMapTexture.Reset();
    m_DepthTexture.Reset();
    m_DefaultNormal.Reset();
    m_DefaultAlbedo.Reset();
    m_DefaultRoughnessMetallic.Reset();

    m_Device.reset();

#if defined(_DEBUG)
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                         DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif
}

std::unique_ptr<Buffer> Renderer::CreateConstantBuffer(UINT64 size) {
    const BufferDesc desc{
        .Size = size, .HeapType = D3D12_HEAP_TYPE_UPLOAD, .MiscFlags = BufferMiscFlags::ConstantBuffer};
    return std::make_unique<Buffer>(*m_Device, *m_CommandQueue, desc);
}

std::unique_ptr<MeshBuffer> Renderer::CreateMeshBuffer(const Mesh& mesh) {
    return std::make_unique<MeshBuffer>(*m_Device, *m_CommandQueue, mesh);
}

std::unique_ptr<Texture> Renderer::CreateTexture(const Image& image) {
    TextureDesc desc{.Width = image.GetWidth(),
                     .Height = image.GetHeight(),
                     .Format = image.GetDXGIFormat(),
                     .Usage = TextureUsage::ShaderResource};
    auto texture = std::make_unique<Texture>();
    texture->CreateFromImage(*m_Device, *m_CommandQueue, desc, image);
    return texture;
}

void Renderer::EnsureDefaultMaterial() {
    if (m_DefaultAlbedo.GetSrvIndex() != INVALID_BINDLESS_INDEX)
        return;
    static constexpr uint8_t kWhitePixel[]{255, 255, 255, 255};
    static constexpr uint8_t kFlatNormalPixel[]{128, 128, 255, 255};
    static constexpr uint8_t kDefaultRoughnessMetallicPixel[]{255, 255, 0, 255};
    const Image white = Image::FromRawRGBA(kWhitePixel, 1, 1);
    const Image flatNormal = Image::FromRawRGBA(kFlatNormalPixel, 1, 1);
    const Image defaultRoughnessMetallic = Image::FromRawRGBA(kDefaultRoughnessMetallicPixel, 1, 1);
    m_DefaultAlbedo.CreateFromImage(
        *m_Device, *m_CommandQueue,
        {.Width = 1, .Height = 1, .Format = white.GetDXGIFormat(), .Usage = TextureUsage::ShaderResource}, white);
    m_DefaultNormal.CreateFromImage(
        *m_Device, *m_CommandQueue,
        {.Width = 1, .Height = 1, .Format = flatNormal.GetDXGIFormat(), .Usage = TextureUsage::ShaderResource},
        flatNormal);
    m_DefaultRoughnessMetallic.CreateFromImage(*m_Device, *m_CommandQueue,
                                               {.Width = 1,
                                                .Height = 1,
                                                .Format = defaultRoughnessMetallic.GetDXGIFormat(),
                                                .Usage = TextureUsage::ShaderResource},
                                               defaultRoughnessMetallic);
    m_DefaultMaterial = {.AlbedoIndex = m_DefaultAlbedo.GetSrvIndex(),
                         .NormalIndex = m_DefaultNormal.GetSrvIndex(),
                         .RoughnessMetallicIndex = m_DefaultRoughnessMetallic.GetSrvIndex()};
}

const Renderer::ModelResources& Renderer::EnsureModelResources(const Model& model) {
    auto [it, inserted] = m_ModelCache.try_emplace(&model);
    if (!inserted)
        return it->second;

    ModelResources& resources = it->second;
    resources.Meshes.reserve(model.Meshes.size());
    for (const auto& mesh : model.Meshes)
        resources.Meshes.push_back(CreateMeshBuffer(mesh));

    EnsureDefaultMaterial();
    resources.Materials.reserve(model.Materials.size());
    resources.Textures.reserve(model.Materials.size() * 3);
    for (const auto& material : model.Materials) {
        auto albedo = material.Albedo ? CreateTexture(*material.Albedo) : nullptr;
        auto normal = material.Normal ? CreateTexture(*material.Normal) : nullptr;
        auto roughnessMetallic = material.RoughnessMetallic ? CreateTexture(*material.RoughnessMetallic) : nullptr;
        resources.Materials.push_back({
            .AlbedoIndex = albedo ? albedo->GetSrvIndex() : m_DefaultMaterial.AlbedoIndex,
            .NormalIndex = normal ? normal->GetSrvIndex() : m_DefaultMaterial.NormalIndex,
            .RoughnessMetallicIndex =
                roughnessMetallic ? roughnessMetallic->GetSrvIndex() : m_DefaultMaterial.RoughnessMetallicIndex,
        });
        if (albedo)
            resources.Textures.push_back(std::move(albedo));
        if (normal)
            resources.Textures.push_back(std::move(normal));
        if (roughnessMetallic)
            resources.Textures.push_back(std::move(roughnessMetallic));
    }
    return it->second;
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence->Flush(m_CommandQueue->GetHandle());
    m_SwapChain->OnResize(width, height);

    CreateRenderTargets(width, height);
}

void Renderer::Render(const Scene& scene) {
    auto currentIdx{m_SwapChain->GetCurrentBackBufferIndex()};
    auto& frame{m_FrameResources[currentIdx]};

    m_Fence->WaitForValue(frame.FenceValue);

    Texture& backBuffer{m_SwapChain->GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList{frame.CommandList->GetHandle()};

    // Build one RenderItem per entity mesh; GPU resources are uploaded on first use and cached per Model asset.
    auto& objectConstantBuffers{frame.ObjectConstantBuffers};
    m_RenderItems.clear();
    uint32_t objectIndex{};
    for (const auto& [entity, modelComponent] : scene.GetEntityRegistry().View<ModelComponent>()) {
        if (!modelComponent.Model.IsValid())
            continue;

        const Model* model = scene.GetAssetManager().GetModel(modelComponent.Model);
        if (!model)
            continue;

        if (objectIndex >= objectConstantBuffers.size())
            objectConstantBuffers.resize(objectIndex + 1);
        if (!objectConstantBuffers[objectIndex])
            objectConstantBuffers[objectIndex] = CreateConstantBuffer(sizeof(DirectX::XMFLOAT4X4));

        DirectX::XMFLOAT4X4 worldMatrix;
        const Transform* transform = scene.GetEntityRegistry().GetComponent<Transform>(entity);
        DirectX::XMStoreFloat4x4(&worldMatrix, transform ? transform->GetMatrix() : DirectX::XMMatrixIdentity());
        objectConstantBuffers[objectIndex]->Write(&worldMatrix, sizeof(worldMatrix));

        const ModelResources& resources = EnsureModelResources(*model);
        for (size_t m{}; m < model->Meshes.size(); ++m) {
            m_RenderItems.push_back({
                .Mesh = resources.Meshes[m].get(),
                .TransformCB = objectConstantBuffers[objectIndex].get(),
                .Material = resources.Materials[model->Meshes[m].MaterialIndex],
                .ShadowCaster = modelComponent.CastsShadow,
            });
        }
        ++objectIndex;
    }

    SceneInfo sceneInfo = scene.GetSceneInfo();
    sceneInfo.ScreenResolution[0] = m_SwapChain->GetWidth();
    sceneInfo.ScreenResolution[1] = m_SwapChain->GetHeight();
    const CascadedShadowMapsData cascadedShadowMapsData = scene.GetCascadedShadowMapsData();

    std::vector<LightData> lightData;
    lightData.reserve(1 + scene.GetPointLights().size() + scene.GetSpotLights().size());
    {
        const DirectionalLight& directionalLight = scene.GetDirectionalLight();
        lightData.push_back({
            .Position = {},
            .Type = static_cast<uint32_t>(LightType::Directional),
            .Direction = directionalLight.Direction,
            .Color = directionalLight.Color,
            .Intensity = directionalLight.Intensity,
        });
    }
    for (const PointLight& pointLight : scene.GetPointLights()) {
        lightData.push_back({
            .Position = pointLight.Position,
            .Type = static_cast<uint32_t>(LightType::Point),
            .Color = pointLight.Color,
            .Intensity = pointLight.Intensity,
        });
    }
    for (const SpotLight& spotLight : scene.GetSpotLights()) {
        lightData.push_back({
            .Position = spotLight.Position,
            .Type = static_cast<uint32_t>(LightType::Spot),
            .Direction = spotLight.Direction,
            .Color = spotLight.Color,
            .Intensity = spotLight.Intensity,
            .CosInnerCone = std::cos(DirectX::XMConvertToRadians(spotLight.InnerConeAngle)),
            .CosOuterCone = std::cos(DirectX::XMConvertToRadians(spotLight.OuterConeAngle)),
        });
    }
    if (lightData.size() > kMaxLights) {
        assert(false && "Too many lights; excess lights beyond kMaxLights are ignored.");
        lightData.resize(kMaxLights);
    }
    frame.LightDataStructuredBuffer->Write(lightData.data(), lightData.size() * sizeof(LightData));
    sceneInfo.LightCount = static_cast<uint32_t>(lightData.size());
    sceneInfo.LightIndex = frame.LightDataStructuredBuffer->GetSrvIndex();

    frame.SceneInfoConstantBuffer->Write(&sceneInfo, sizeof(sceneInfo));
    frame.CascadedShadowMapsDataConstantBuffer->Write(&cascadedShadowMapsData, sizeof(cascadedShadowMapsData));

    m_Device->SetDescriptorHeaps(*frame.CommandList);

    // Cascaded shadow maps
    {
        m_ShadowMapTexture.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        m_ShadowPass->OnRender(*frame.CommandList, m_ShadowMapTexture, *frame.CascadedShadowMapsDataConstantBuffer,
                               cascadedShadowMapsData.CascadeCount, m_RenderItems);
    }

    // Forward lighting pass
    {
        m_HdrTexture.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_ShadowMapTexture.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        m_ForwardLighting->OnRender(*frame.CommandList, m_HdrTexture, m_DepthTexture, m_ShadowMapTexture,
                                    *frame.SceneInfoConstantBuffer, *frame.CascadedShadowMapsDataConstantBuffer,
                                    m_RenderItems);
    }

    // Skybox pass
    const Skybox& skybox = scene.GetSkybox();
    if (skybox.Panorama.GetSrvIndex() != INVALID_BINDLESS_INDEX) {
        m_SkyboxPass->OnRender(*frame.CommandList, *m_SkyboxMeshBuffer, m_HdrTexture, m_DepthTexture,
                               skybox.Panorama.GetSrvIndex(), *frame.SceneInfoConstantBuffer);
    }

    m_RenderItems.clear();

    // Post-processing
    {
        m_HdrTexture.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_PresentTarget.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_ToneMapPass->Dispatch(*frame.CommandList, m_HdrTexture.GetSrvIndex(), m_PresentTarget.GetUavIndex(),
                                *frame.SceneInfoConstantBuffer, m_SwapChain->GetWidth(), m_SwapChain->GetHeight());
    }

    // Present target
    {
        m_PresentTarget.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        backBuffer.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_COPY_DEST);

        cmdList->CopyResource(backBuffer.GetResource(), m_PresentTarget.GetResource());

        backBuffer.Transition(*frame.CommandList, D3D12_RESOURCE_STATE_PRESENT);
    }

    frame.CommandList->Close();
    ID3D12CommandList* const ppCommandLists[]{cmdList};
    m_CommandQueue->ExecuteCommandLists(ppCommandLists);

    UINT syncInterval{m_VSync || !m_TearingSupported ? 1u : 0u};
    UINT presentFlags{!m_VSync && m_TearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0u};
    ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

    frame.FenceValue = m_Fence->Signal(m_CommandQueue->GetHandle());
}

} // namespace GEngine
