#include "PCH.hpp"

#include "Rendering/Renderer.hpp"

#include "Core/Utility/Math.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Interop/Common.h"
#include "Interop/Light.h"
#include "Rendering/Culling.hpp"
#include "UI/ImGui/Backends/D3D12/ImGuiD3D12RendererBackend.hpp"

#include <algorithm>
#include <cstring>

namespace GEngine {

using namespace Microsoft::WRL;

namespace {

void TransitionResource(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* resource,
                        const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after) {
    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after)};
    cmdList->ResourceBarrier(1, &barrier);
}

void WriteDynamicBuffer(const Buffer& buffer, const void* data, const UINT64 size) {
    void* const dst = buffer.Map();
    std::memcpy(dst, data, size);
    buffer.Unmap();
}

[[nodiscard]] constexpr TextureDesc DepthStencilTargetDesc(uint32_t width, uint32_t height) noexcept {
    return TextureDesc{
        .Width = width,
        .Height = height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .Usage = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
        .ClearValue = {.Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {.Depth = 1.0f, .Stencil = 0}},
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
    };
}

constexpr RenderPass::GBufferFormats kGBufferFormats{};

[[nodiscard]] constexpr TextureDesc GBufferTargetDesc(uint32_t width, uint32_t height,
                                                      const DXGI_FORMAT format) noexcept {
    return TextureDesc{
        .Width = width,
        .Height = height,
        .Format = format,
        .Usage = TextureUsage::RenderTarget | TextureUsage::ShaderResource,
        .ClearValue = {.Format = format, .Color = {0.0f, 0.0f, 0.0f, 0.0f}},
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    };
}

[[nodiscard]] RenderPass::GBufferTextures CreateGBufferTextures(Device& device, uint32_t width, uint32_t height) {
    return {
        Texture(device, GBufferTargetDesc(width, height, kGBufferFormats.AlbedoFormat)),
        Texture(device, GBufferTargetDesc(width, height, kGBufferFormats.NormalFormat)),
        Texture(device, GBufferTargetDesc(width, height, kGBufferFormats.RoughnessMetallicFormat)),
    };
}

[[nodiscard]] constexpr TextureDesc HdrTargetDesc(uint32_t width, uint32_t height) noexcept {
    return TextureDesc{
        .Width = width,
        .Height = height,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .Usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget | TextureUsage::UnorderedAccess,
        .ClearValue = {.Format = DXGI_FORMAT_R16G16B16A16_FLOAT, .Color = {0.4f, 0.6f, 0.9f, 1.0f}},
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    };
}

[[nodiscard]] constexpr TextureDesc BloomTargetDesc(uint32_t width, uint32_t height) noexcept {
    return TextureDesc{
        .Width = width,
        .Height = height,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .Usage = TextureUsage::ShaderResource | TextureUsage::UnorderedAccess,
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    };
}

constexpr uint32_t kAmbientOcclusionScale = 2;

[[nodiscard]] constexpr TextureDesc AmbientOcclusionTargetDesc(uint32_t width, uint32_t height) noexcept {
    return TextureDesc{
        .Width = std::max(1u, width / kAmbientOcclusionScale),
        .Height = std::max(1u, height / kAmbientOcclusionScale),
        .Format = DXGI_FORMAT_R32_FLOAT,
        .Usage = TextureUsage::ShaderResource | TextureUsage::UnorderedAccess,
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    };
}

[[nodiscard]] constexpr TextureDesc PresentTargetDesc(uint32_t width, uint32_t height) noexcept {
    return TextureDesc{
        .Width = width,
        .Height = height,
        .Format = SwapChain::BackBufferFormat,
        .Usage = TextureUsage::UnorderedAccess,
        .InitialState = D3D12_RESOURCE_STATE_COPY_SOURCE,
    };
}

[[nodiscard]] constexpr TextureDesc ShadowMapTargetDesc(uint32_t size) noexcept {
    return TextureDesc{
        .Width = size,
        .Height = size,
        .DepthOrArraySize = kMaxCascades,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .Usage = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
        .ClearValue = {.Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {.Depth = 1.0f, .Stencil = 0}},
        .InitialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    };
}

constexpr size_t kTransientLinearAllocatorSize = 16 * 1024 * 1024;

} // namespace

Renderer::Renderer(HWND hwnd, uint32_t width, uint32_t height, bool useWarp, uint32_t shadowMapSize)
    : m_Device(useWarp), m_CommandQueue(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT), m_Fence(m_Device),
      m_SwapChain(m_Device, hwnd, m_CommandQueue, width, height, SwapChain::NumFrames), m_UploadEngine(m_Device),
      m_GpuResources(m_Device, m_UploadEngine), m_FrameResources(CreateFrameResources()),
      m_HdrTexture(Texture(m_Device, HdrTargetDesc(width, height))),
      m_BloomTexture(Texture(m_Device, BloomTargetDesc(width, height))),
      m_PresentTarget(Texture(m_Device, PresentTargetDesc(width, height))),
      m_AmbientOcclusionTexture(Texture(m_Device, AmbientOcclusionTargetDesc(width, height))),
      m_BlurredAmbientOcclusionTexture(Texture(m_Device, AmbientOcclusionTargetDesc(width, height))),
      m_DepthTexture(Texture(m_Device, DepthStencilTargetDesc(width, height))),
      m_ShadowMapTexture(Texture(m_Device, ShadowMapTargetDesc(shadowMapSize))),
      m_GBufferTextures(CreateGBufferTextures(m_Device, width, height)),
      m_ShadowPass(m_Device, m_ShadowMapTexture.GetDesc().Format),
      m_ForwardLightingPass(m_Device, m_HdrTexture.GetDesc().Format, m_DepthTexture.GetDesc().Format),
      m_GBufferPass(m_Device, kGBufferFormats, m_DepthTexture.GetDesc().Format), m_SSAOPass(m_Device, m_UploadEngine),
      m_GaussianBlurPass(m_Device), m_DeferredLightingPass(m_Device), m_BloomPass(m_Device), m_ToneMapPass(m_Device),
      m_SkyboxRenderer(m_Device, m_UploadEngine, m_GpuResources, m_HdrTexture.GetDesc().Format,
                       m_DepthTexture.GetDesc().Format) {}

Renderer::FrameResources Renderer::CreateFrameResources() {
    std::array<FrameResource, SwapChain::NumFrames> frame;
    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        FrameResource& frameResource = frame[i];
        ThrowIfFailed(m_Device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                             IID_PPV_ARGS(&frameResource.CommandAllocator)));
        frameResource.CommandList = std::make_unique<CommandList>(m_Device, frameResource.CommandAllocator.Get(),
                                                                  D3D12_COMMAND_LIST_TYPE_DIRECT);

        const BufferDesc sceneInfoCbDesc{.Size = sizeof(SceneInfo),
                                         .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                         .MiscFlags = BufferMiscFlags::ConstantBuffer};
        frameResource.SceneInfoConstantBuffer = std::make_unique<Buffer>(m_Device, sceneInfoCbDesc);

        const BufferDesc gaussianBlurWeightsCbDesc{.Size = sizeof(GaussianWeights),
                                                   .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                                   .MiscFlags = BufferMiscFlags::ConstantBuffer};
        frameResource.GaussianBlurWeightsConstantBuffer = std::make_unique<Buffer>(m_Device, gaussianBlurWeightsCbDesc);

        const BufferDesc bloomWeightsCbDesc{.Size = sizeof(BloomWeights),
                                            .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                            .MiscFlags = BufferMiscFlags::ConstantBuffer};
        frameResource.BloomWeightsConstantBuffer = std::make_unique<Buffer>(m_Device, bloomWeightsCbDesc);

        const BufferDesc cascadedShadowMapsDataCbDesc{.Size = sizeof(CascadedShadowMapsData),
                                                      .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                                      .MiscFlags = BufferMiscFlags::ConstantBuffer};
        frameResource.CascadedShadowMapsDataConstantBuffer =
            std::make_unique<Buffer>(m_Device, cascadedShadowMapsDataCbDesc);

        const BufferDesc LightDataSbDesc{.Size = kMaxLights * sizeof(LightData), .HeapType = D3D12_HEAP_TYPE_UPLOAD};
        frameResource.LightDataStructuredBuffer = std::make_unique<Buffer>(m_Device, LightDataSbDesc);
        frameResource.LightDataStructuredBuffer->CreateStructuredBufferSRV(m_Device, kMaxLights, sizeof(LightData));

        frameResource.TransientLinearAllocator =
            std::make_unique<LinearAllocator>(m_Device, kTransientLinearAllocatorSize);
    }
    return frame;
}

void Renderer::CreateRenderTargets(uint32_t width, uint32_t height) {
    m_DepthTexture.Create(m_Device, DepthStencilTargetDesc(width, height));
    m_HdrTexture.Create(m_Device, HdrTargetDesc(width, height));
    m_BloomTexture.Create(m_Device, BloomTargetDesc(width, height));
    m_PresentTarget.Create(m_Device, PresentTargetDesc(width, height));
    m_AmbientOcclusionTexture.Create(m_Device, AmbientOcclusionTargetDesc(width, height));
    m_BlurredAmbientOcclusionTexture.Create(m_Device, AmbientOcclusionTargetDesc(width, height));
    m_GBufferTextures = CreateGBufferTextures(m_Device, width, height);
}

void Renderer::Destroy() {
    Flush();

    for (auto& frame : m_FrameResources) {
        frame.CommandList.reset();
        frame.CommandAllocator.Reset();
        frame.SceneInfoConstantBuffer.reset();
        frame.GaussianBlurWeightsConstantBuffer.reset();
        frame.BloomWeightsConstantBuffer.reset();
        frame.CascadedShadowMapsDataConstantBuffer.reset();
        frame.LightDataStructuredBuffer.reset();
        frame.TransientLinearAllocator.reset();
    }

    m_PresentTarget.Reset();
    m_HdrTexture.Reset();
    m_BloomTexture.Reset();
    m_AmbientOcclusionTexture.Reset();
    m_BlurredAmbientOcclusionTexture.Reset();
    m_ShadowMapTexture.Reset();
    m_DepthTexture.Reset();
}

void Renderer::Flush() {
    m_Fence.Flush(m_CommandQueue.GetHandle());
}

std::unique_ptr<UI::ImGuiRendererBackend> Renderer::CreateUiRendererBackend() {
    return std::make_unique<UI::ImGuiD3D12RendererBackend>(m_Device, m_CommandQueue,
                                                           m_Device.GetShaderResourceDescriptorHeap(),
                                                           SwapChain::NumFrames, SwapChain::BackBufferFormat);
}

void Renderer::ReportLiveObjects() {
#if defined(_DEBUG)
    ComPtr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
                                     DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
#endif
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence.Flush(m_CommandQueue.GetHandle());
    m_SwapChain.OnResize(width, height);

    CreateRenderTargets(width, height);
}

void Renderer::UpdateGpuScene(const Scene& scene, const AssetManager& assetManager) {
    bool uploadBatchOpen = false;
    auto openBatch = [&]() {
        if (!uploadBatchOpen) {
            m_UploadEngine.Begin();
            uploadBatchOpen = true;
        }
    };

    for (const auto& [entity, modelComponent] : scene.GetEntityRegistry().View<ModelComponent>()) {
        if (!modelComponent.Model.IsValid())
            continue;
        if (m_GpuResources.HasModel(modelComponent.Model.Id))
            continue;

        const Model* model = assetManager.GetModel(modelComponent.Model);
        if (!model)
            continue;

        openBatch();
        m_GpuResources.StageModel(*model, modelComponent.Model.Id);
    }

    if (uploadBatchOpen)
        m_UploadEngine.Submit();

    m_SkyboxRenderer.Update(scene.GetSkybox());
}

void Renderer::UploadSceneData(const Scene& scene, const CascadedShadowMapsData& cascades, FrameResource& frame) {
    SceneInfo sceneInfo = scene.GetSceneInfo();
    sceneInfo.screenResolution.x = m_SwapChain.GetWidth();
    sceneInfo.screenResolution.y = m_SwapChain.GetHeight();
    sceneInfo.irradianceIndex = m_SkyboxRenderer.GetIrradianceSrvIndex();
    const std::vector<LightData> lightData = scene.GetLightData();

    WriteDynamicBuffer(*frame.LightDataStructuredBuffer, lightData.data(), lightData.size() * sizeof(LightData));
    sceneInfo.lightCount = static_cast<uint32_t>(lightData.size());
    sceneInfo.lightIndex = frame.LightDataStructuredBuffer->GetSrvIndex();

    WriteDynamicBuffer(*frame.SceneInfoConstantBuffer, &sceneInfo, sizeof(sceneInfo));
    WriteDynamicBuffer(*frame.CascadedShadowMapsDataConstantBuffer, &cascades, sizeof(cascades));
}

void Renderer::CreateRenderItems(const Scene& scene, LinearAllocator& transientAllocator,
                                 std::vector<RenderItem>& renderItems) {
    renderItems.clear();
    for (const auto& [entity, modelComponent] : scene.GetEntityRegistry().View<ModelComponent>()) {
        if (!modelComponent.Model.IsValid())
            continue;

        const GpuModel* gpuModel = m_GpuResources.FindModel(modelComponent.Model.Id);
        if (!gpuModel)
            continue;

        auto [objectConstantBufferPtr, objectConstantBufferGpuAddress] =
            transientAllocator.Allocate(sizeof(ObjectData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        ObjectData objectData{};
        const Transform* transform = scene.GetEntityRegistry().GetComponent<Transform>(entity);
        const DirectX::XMMATRIX worldMatrix = transform ? transform->GetMatrix() : DirectX::XMMatrixIdentity();
        const DirectX::XMMATRIX normalMatrix =
            DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, worldMatrix));
        DirectX::XMStoreFloat4x4(&objectData.world, worldMatrix);
        DirectX::XMStoreFloat4x4(&objectData.normal, normalMatrix);
        std::memcpy(objectConstantBufferPtr, &objectData, sizeof(ObjectData));

        for (const GpuMesh& mesh : gpuModel->Meshes) {
            DirectX::BoundingBox worldBounds;
            mesh.BoundingBox.Transform(worldBounds, worldMatrix);

            renderItems.push_back({
                .Mesh = &mesh.Geometry,
                .ObjectConstantBuffer = objectConstantBufferGpuAddress,
                .Material = mesh.Material,
                .BoundingBox = worldBounds,
                .ShadowCaster = modelComponent.CastsShadow,
            });
        }
    }
}

void Renderer::Render(const Scene& scene, const AssetManager& assetManager, UI::Overlay* overlay) {
    FrameResource& frame{m_FrameResources[m_SwapChain.GetCurrentBackBufferIndex()]};

    m_Fence.WaitForValue(frame.FenceValue);

    Texture& backBuffer{m_SwapChain.GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    ID3D12GraphicsCommandList4* cmdList{frame.CommandList->GetHandle()};

    const CascadedShadowMapsData cascades = scene.GetCascadedShadowMapsData();

    frame.TransientLinearAllocator->Reset();

    UpdateGpuScene(scene, assetManager);
    UploadSceneData(scene, cascades, frame);

    CreateRenderItems(scene, *frame.TransientLinearAllocator, m_RenderLists.RenderItems);

    CullView(m_RenderLists.RenderItems, Frustum::FromViewProjection(scene.GetActiveCamera().GetViewProjectionMatrix()),
             m_RenderLists.CameraVisibleItems);
    CullShadowCascades(m_RenderLists.RenderItems, cascades, m_RenderLists.ShadowCascades);

    m_Device.SetDescriptorHeaps(*frame.CommandList);

    m_SkyboxRenderer.BakeEnvironmentMaps(*frame.CommandList);

    // Cascaded shadow maps
    {
        TransitionResource(cmdList, m_ShadowMapTexture.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_DEPTH_WRITE);

        m_ShadowPass.OnRender(*frame.CommandList, m_ShadowMapTexture, *frame.CascadedShadowMapsDataConstantBuffer,
                              m_RenderLists.RenderItems, m_RenderLists.ShadowCascades);
    }

    if (m_Settings.Mode == RenderMode::Forward) {
        TransitionResource(cmdList, m_ShadowMapTexture.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_ForwardLightingPass.OnRender(*frame.CommandList, m_HdrTexture, m_DepthTexture, m_ShadowMapTexture,
                                       *frame.SceneInfoConstantBuffer, *frame.CascadedShadowMapsDataConstantBuffer,
                                       m_RenderLists.RenderItems, m_RenderLists.CameraVisibleItems);
    } else {
        TransitionResource(cmdList, m_GBufferTextures.AlbedoTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        TransitionResource(cmdList, m_GBufferTextures.NormalTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        TransitionResource(cmdList, m_GBufferTextures.RoughnessMetallicTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        m_GBufferPass.OnRender(*frame.CommandList, m_GBufferTextures, m_DepthTexture, *frame.SceneInfoConstantBuffer,
                               m_RenderLists.RenderItems, m_RenderLists.CameraVisibleItems);

        TransitionResource(cmdList, m_GBufferTextures.AlbedoTexture.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_GBufferTextures.NormalTexture.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_GBufferTextures.RoughnessMetallicTexture.GetResource(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_DepthTexture.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_ShadowMapTexture.GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        if (!frame.HasUploadedGaussianBlurWeights || frame.UploadedGaussianBlurSettings != m_Settings.SSAOBlur) {
            m_GaussianBlurPass.UploadWeights(*frame.GaussianBlurWeightsConstantBuffer, m_Settings.SSAOBlur);
            frame.UploadedGaussianBlurSettings = m_Settings.SSAOBlur;
            frame.HasUploadedGaussianBlurWeights = true;
        }

        TransitionResource(cmdList, m_AmbientOcclusionTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_SSAOPass.Dispatch(*frame.CommandList, m_GBufferTextures.NormalTexture.GetSrvIndex(),
                            m_DepthTexture.GetSrvIndex(), m_AmbientOcclusionTexture, *frame.SceneInfoConstantBuffer,
                            m_Settings.SSAO);
        TransitionResource(cmdList, m_AmbientOcclusionTexture.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        TransitionResource(cmdList, m_BlurredAmbientOcclusionTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_GaussianBlurPass.Dispatch(*frame.CommandList, m_AmbientOcclusionTexture.GetSrvIndex(),
                                    m_BlurredAmbientOcclusionTexture, true, *frame.SceneInfoConstantBuffer,
                                    *frame.GaussianBlurWeightsConstantBuffer, m_Settings.SSAOBlur);
        TransitionResource(cmdList, m_BlurredAmbientOcclusionTexture.GetResource(),
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_AmbientOcclusionTexture.GetResource(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_GaussianBlurPass.Dispatch(*frame.CommandList, m_BlurredAmbientOcclusionTexture.GetSrvIndex(),
                                    m_AmbientOcclusionTexture, false, *frame.SceneInfoConstantBuffer,
                                    *frame.GaussianBlurWeightsConstantBuffer, m_Settings.SSAOBlur);
        TransitionResource(cmdList, m_AmbientOcclusionTexture.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_DeferredLightingPass.Dispatch(
            *frame.CommandList, m_GBufferTextures.AlbedoTexture.GetSrvIndex(),
            m_GBufferTextures.NormalTexture.GetSrvIndex(), m_GBufferTextures.RoughnessMetallicTexture.GetSrvIndex(),
            m_AmbientOcclusionTexture.GetSrvIndex(), m_DepthTexture.GetSrvIndex(), m_ShadowMapTexture.GetSrvIndex(),
            m_HdrTexture.GetUavIndex(), *frame.SceneInfoConstantBuffer, *frame.CascadedShadowMapsDataConstantBuffer,
            m_SwapChain.GetWidth(), m_SwapChain.GetHeight());

        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        TransitionResource(cmdList, m_DepthTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_DEPTH_WRITE);
        TransitionResource(cmdList, m_ShadowMapTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // Skybox pass
    m_SkyboxRenderer.Render(*frame.CommandList, m_HdrTexture, m_DepthTexture, *frame.SceneInfoConstantBuffer);

    // Post-processing
    {
        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_BloomTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        if (!frame.HasUploadedBloomWeights || frame.UploadedBloomWeightsRevision != m_Settings.Bloom.WeightsRevision) {
            m_BloomPass.UploadWeights(*frame.BloomWeightsConstantBuffer, m_Settings.Bloom);
            frame.UploadedBloomWeightsRevision = m_Settings.Bloom.WeightsRevision;
            frame.HasUploadedBloomWeights = true;
        }

        // Bloom horizontal pass, writing to the bloom texture
        m_BloomPass.Dispatch(*frame.CommandList, m_HdrTexture.GetSrvIndex(), m_BloomTexture.GetUavIndex(), true,
                             *frame.SceneInfoConstantBuffer, m_SwapChain.GetWidth(), m_SwapChain.GetHeight(),
                             *frame.BloomWeightsConstantBuffer, m_Settings.Bloom);

        TransitionResource(cmdList, m_BloomTexture.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Bloom vertical pass, writing directly to the HDR texture
        m_BloomPass.Dispatch(*frame.CommandList, m_BloomTexture.GetSrvIndex(), m_HdrTexture.GetUavIndex(), false,
                             *frame.SceneInfoConstantBuffer, m_SwapChain.GetWidth(), m_SwapChain.GetHeight(),
                             *frame.BloomWeightsConstantBuffer, m_Settings.Bloom);

        TransitionResource(cmdList, m_HdrTexture.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionResource(cmdList, m_PresentTarget.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                           D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Tone mapping
        m_ToneMapPass.Dispatch(*frame.CommandList, m_HdrTexture.GetSrvIndex(), m_PresentTarget.GetUavIndex(),
                               *frame.SceneInfoConstantBuffer, m_SwapChain.GetWidth(), m_SwapChain.GetHeight(),
                               m_Settings.ToneMapping);
    }

    // Present target
    {
        TransitionResource(cmdList, m_PresentTarget.GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                           D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionResource(cmdList, backBuffer.GetResource(), D3D12_RESOURCE_STATE_PRESENT,
                           D3D12_RESOURCE_STATE_COPY_DEST);

        cmdList->CopyResource(backBuffer.GetResource(), m_PresentTarget.GetResource());

        TransitionResource(cmdList, backBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (overlay) {
            const D3D12_CPU_DESCRIPTOR_HANDLE rtv = backBuffer.GetRtvHandle();
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            overlay->Render(cmdList);
        }
        TransitionResource(cmdList, backBuffer.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PRESENT);
    }

    frame.CommandList->Close();
    ID3D12CommandList* const ppCommandLists[]{cmdList};
    m_CommandQueue.ExecuteCommandLists(ppCommandLists);

    ThrowIfFailed(m_SwapChain.Present());

    frame.FenceValue = m_Fence.Signal(m_CommandQueue.GetHandle());
}

} // namespace GEngine
