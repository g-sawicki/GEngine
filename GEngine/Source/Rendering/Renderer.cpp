#include "PCH.hpp"

#include "Rendering/Renderer.hpp"

#include "Core/Utility/Math.hpp"
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
    m_UploadQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_GPUResourceManager = std::make_unique<GPUResourceManager>(*m_Device, *m_UploadQueue);

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

    m_GPUResourceManager->BeginCopyPass();
    m_SkyboxMeshBuffer = m_GPUResourceManager->StageMeshBuffer(MeshFactory::Cube());
    m_GPUResourceManager->EndAndSubmitCopyPass();
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
    m_ToneMapPass.reset();
    m_ForwardLighting.reset();
    m_ShadowPass.reset();
    m_SkyboxPass.reset();
    m_SkyboxMeshBuffer.reset();
    m_SkyboxTexture.reset();
    m_GPUResourceManager->Shutdown();
    m_GPUResourceManager.reset();
    m_UploadQueue.reset();
    m_CommandQueue.reset();

    m_PresentTarget.Reset();
    m_HdrTexture.Reset();
    m_ShadowMapTexture.Reset();
    m_DepthTexture.Reset();

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

void Renderer::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence->Flush(m_CommandQueue->GetHandle());
    m_SwapChain->OnResize(width, height);

    CreateRenderTargets(width, height);
}

void Renderer::FlushPendingUploads(const Scene& scene, const AssetManager& assetManager) {
    bool copyPassOpen = false;
    auto openPass = [&]() {
        if (!copyPassOpen) {
            m_GPUResourceManager->BeginCopyPass();
            copyPassOpen = true;
        }
    };

    for (const auto& [entity, modelComponent] : scene.GetEntityRegistry().View<ModelComponent>()) {
        if (!modelComponent.Model.IsValid())
            continue;
        if (m_GPUResourceManager->GetGPUHandle(modelComponent.Model).IsValid())
            continue;

        const Model* model = assetManager.GetModel(modelComponent.Model);
        if (!model)
            continue;

        openPass();
        const GPUModelHandle gpuModel = m_GPUResourceManager->StageModelToVRAM(*model);
        m_GPUResourceManager->RegisterModelHandle(modelComponent.Model, gpuModel);
    }

    const Image& panorama = scene.GetSkybox().Panorama;
    if (m_SkyboxSource != &panorama && panorama.GetWidth() != 0) {
        openPass();
        m_SkyboxTexture = m_GPUResourceManager->StageTexture(panorama, /*isSRGB*/ false);
        m_SkyboxSource = &panorama;
    }

    if (copyPassOpen) {
        m_GPUResourceManager->EndAndSubmitCopyPass();
    }
}

void Renderer::Render(const Scene& scene, const AssetManager& assetManager) {
    auto currentIdx{m_SwapChain->GetCurrentBackBufferIndex()};
    auto& frame{m_FrameResources[currentIdx]};

    m_Fence->WaitForValue(frame.FenceValue);

    Texture& backBuffer{m_SwapChain->GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList{frame.CommandList->GetHandle()};

    FlushPendingUploads(scene, assetManager);

    auto& objectConstantBuffers{frame.ObjectConstantBuffers};
    m_RenderItems.clear();
    uint32_t objectIndex{};
    for (const auto& [entity, modelComponent] : scene.GetEntityRegistry().View<ModelComponent>()) {
        if (!modelComponent.Model.IsValid())
            continue;

        const Model* model = assetManager.GetModel(modelComponent.Model);
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

        const GPUModelHandle gpuModel = m_GPUResourceManager->GetGPUHandle(modelComponent.Model);
        if (!gpuModel.IsValid())
            continue;

        const uint32_t submeshCount = m_GPUResourceManager->GetSubmeshCount(gpuModel);
        for (uint32_t m{}; m < submeshCount; ++m) {
            const GPUSubmesh& submesh = m_GPUResourceManager->GetSubmesh(gpuModel, m);
            m_RenderItems.push_back({
                .Mesh = submesh.Mesh.get(),
                .TransformCB = objectConstantBuffers[objectIndex].get(),
                .Material = submesh.Material,
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
    if (m_SkyboxTexture && m_SkyboxTexture->GetSrvIndex() != INVALID_BINDLESS_INDEX) {
        m_SkyboxPass->OnRender(*frame.CommandList, *m_SkyboxMeshBuffer, m_HdrTexture, m_DepthTexture,
                               m_SkyboxTexture->GetSrvIndex(), *frame.SceneInfoConstantBuffer);
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
