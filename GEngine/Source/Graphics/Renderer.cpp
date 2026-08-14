#include "PCH.hpp"

#include "Graphics/Renderer.hpp"

#include "Core/Utility/Common.hpp"
#include "Graphics/D3D12/D3D12Common.hpp"
#include "Rendering/MeshBuffer.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

void Renderer::Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp) {
    Device::EnableDebugLayer();
    ComPtr<IDXGIFactory6> dxgiFactory = Device::CreateDXGIFactory();
    ComPtr<IDXGIAdapter4> dxgiAdapter4{Device::GetAdapter(dxgiFactory.Get(), useWarp)};
    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());

    m_RTVDescriptorHeap = DescriptorHeap(*m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChain::NumFrames);
    m_DSVDescriptorHeap = DescriptorHeap(*m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2);
    m_CbvSrvUavDescriptorHeap = DescriptorHeap(*m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024,
                                               D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        std::make_unique<SwapChain>(dxgiFactory.Get(), hwnd, *m_CommandQueue, width, height, SwapChain::NumFrames);

    m_DepthStencilBuffer = CreateDepthBuffer(width, height);
    m_ShadowMapBuffer = CreateDepthBuffer(s_ShadowMapSize, s_ShadowMapSize);

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i)
        m_RTVHandles[i] = m_RTVDescriptorHeap.Allocate().cpuHandle;
    m_DepthStencilView = m_DSVDescriptorHeap.Allocate().cpuHandle;
    m_ShadowMapView = m_DSVDescriptorHeap.Allocate().cpuHandle;

    D3D12_DEPTH_STENCIL_VIEW_DESC shadowDsvDesc{
        .Format = s_DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {.MipSlice = 0},
    };
    m_Device->Get()->CreateDepthStencilView(m_ShadowMapBuffer.Get(), &shadowDsvDesc, m_ShadowMapView);

    CreateShadowMapSRV();

    UpdateRenderTargetViews();

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

        const BufferDesc lightDataCbDesc{.Size = sizeof(LightData),
                                         .HeapType = D3D12_HEAP_TYPE_UPLOAD,
                                         .MiscFlags = BufferMiscFlags::ConstantBuffer};
        m_FrameResources[i].LightDataConstantBuffer =
            std::make_unique<Buffer>(*m_Device, *m_CommandQueue, lightDataCbDesc);
    }

    m_Fence = std::make_unique<Fence>(*m_Device);

    m_ForwardLighting =
        std::make_unique<RenderPass::ForwardLightingPass>(*m_Device, SwapChain::BackBufferFormat, s_DepthStencilFormat);
    m_ShadowPass = std::make_unique<RenderPass::ShadowPass>(*m_Device, s_DepthStencilFormat);
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
        frame.LightDataConstantBuffer.reset();
    }
    m_Fence.reset();
    m_SwapChain.reset();
    m_CommandQueue.reset();
    m_DepthStencilBuffer.Reset();
    m_ShadowMapBuffer.Reset();
    m_ForwardLighting.reset();
    m_ShadowPass.reset();
    m_RTVDescriptorHeap.Release();
    m_DSVDescriptorHeap.Release();
    m_CbvSrvUavDescriptorHeap.Release();
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

std::unique_ptr<Texture> Renderer::CreateTexture(const Image& image) {
    DescriptorHandle handle = m_CbvSrvUavDescriptorHeap.Allocate();
    return std::make_unique<Texture>(*m_Device, *m_CommandQueue, handle, image);
}

std::unique_ptr<MeshBuffer> Renderer::CreateMeshBuffer(const Mesh& mesh) {
    return std::make_unique<MeshBuffer>(*m_Device, *m_CommandQueue, mesh);
}

void Renderer::EnsureDefaultMaterial() {
    if (m_DefaultDiffuse)
        return;
    static constexpr uint8_t kWhitePixel[]{255, 255, 255, 255};
    static constexpr uint8_t kBluePixel[]{0, 0, 255, 255};
    const Image white = Image::FromRawRGBA(kWhitePixel, 1, 1);
    const Image blue = Image::FromRawRGBA(kBluePixel, 1, 1);
    m_DefaultDiffuse = CreateTexture(white);
    m_DefaultSpecular = CreateTexture(white);
    m_DefaultNormal = CreateTexture(blue);
    m_DefaultMaterial = {.DiffuseSRV = m_DefaultDiffuse->GetSRV(),
                         .SpecularSRV = m_DefaultSpecular->GetSRV(),
                         .NormalSRV = m_DefaultNormal->GetSRV()};
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
    resources.Textures.reserve(model.Materials.size() * 2);
    for (const auto& material : model.Materials) {
        auto diffuse = material.Diffuse.has_value() ? CreateTexture(*material.Diffuse) : nullptr;
        auto specular = material.Specular.has_value() ? CreateTexture(*material.Specular) : nullptr;
        auto normal = material.Normal.has_value() ? CreateTexture(*material.Normal) : nullptr;
        resources.Materials.push_back({
            .DiffuseSRV = diffuse ? diffuse->GetSRV() : m_DefaultMaterial.DiffuseSRV,
            .SpecularSRV = specular ? specular->GetSRV() : m_DefaultMaterial.SpecularSRV,
            .NormalSRV = normal ? normal->GetSRV() : m_DefaultMaterial.NormalSRV,
        });
        if (diffuse)
            resources.Textures.push_back(std::move(diffuse));
        if (specular)
            resources.Textures.push_back(std::move(specular));
        if (normal)
            resources.Textures.push_back(std::move(normal));
    }
    return it->second;
}

void Renderer::UpdateRenderTargetViews() {
    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ID3D12Resource* backBuffer{m_SwapChain->GetBackBuffer(i)};
        m_Device->Get()->CreateRenderTargetView(backBuffer, nullptr, m_RTVHandles[i]);
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        .Format = s_DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {.MipSlice = 0},
    };
    m_Device->Get()->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, m_DepthStencilView);
}

void Renderer::CreateShadowMapSRV() {
    DescriptorHandle handle = m_CbvSrvUavDescriptorHeap.Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = DXGI_FORMAT_R32_FLOAT,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = 1},
    };
    m_Device->Get()->CreateShaderResourceView(m_ShadowMapBuffer.Get(), &srvDesc, handle.cpuHandle);
    m_ShadowMapSRV = handle.gpuHandle;
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence->Flush(m_CommandQueue->GetHandle());
    m_SwapChain->OnResize(width, height);

    m_DepthStencilBuffer = CreateDepthBuffer(width, height);

    UpdateRenderTargetViews();
}

ComPtr<ID3D12Resource> Renderer::CreateDepthBuffer(uint32_t width, uint32_t height) {
    D3D12_CLEAR_VALUE clearValue{
        .Format = s_DepthStencilFormat,
        .DepthStencil = {.Depth = 1.0f, .Stencil = 0},
    };

    const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_RESOURCE_DESC desc{CD3DX12_RESOURCE_DESC::Tex2D(s_DepthStencilResourceFormat, width, height, 1, 1, 1,
                                                                  0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)};

    ComPtr<ID3D12Resource> depthBuffer;
    ThrowIfFailed(m_Device->Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                           D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                                                           IID_PPV_ARGS(&depthBuffer)));
    return depthBuffer;
}

void Renderer::Render(const Scene& scene) {
    auto currentIdx{m_SwapChain->GetCurrentBackBufferIndex()};
    auto& frame{m_FrameResources[currentIdx]};

    // Ensure the GPU has finished with this frame's resources before reusing them.
    m_Fence->WaitForValue(frame.FenceValue);

    ID3D12Resource* backBuffer{m_SwapChain->GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList{frame.CommandList->GetHandle()};

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHandles[currentIdx];
    D3D12_CPU_DESCRIPTOR_HANDLE depthHandle = m_DepthStencilView;
    D3D12_CPU_DESCRIPTOR_HANDLE shadowMapHandle = m_ShadowMapView;

    // Build one RenderItem per entity mesh; GPU resources are uploaded on first use and cached per Model asset.
    const auto& entities = scene.GetEntityManager().GetEntities();
    if (m_ObjectConstantBuffers.size() < entities.size())
        m_ObjectConstantBuffers.resize(entities.size());
    m_RenderItems.clear();
    m_RenderItems.reserve(entities.size() * 2);
    for (size_t i{}; i < entities.size(); ++i) {
        const Entity& entity = entities[i];
        if (!entity.Model)
            continue;

        if (!m_ObjectConstantBuffers[i])
            m_ObjectConstantBuffers[i] = CreateConstantBuffer(sizeof(DirectX::XMFLOAT4X4));
        const DirectX::XMFLOAT4X4& worldMatrix = entity.Transform.GetMatrix();
        m_ObjectConstantBuffers[i]->Write(&worldMatrix, sizeof(worldMatrix));

        const Model& model = *entity.Model;
        const ModelResources& resources = EnsureModelResources(model);
        for (size_t m{}; m < model.Meshes.size(); ++m) {
            m_RenderItems.push_back({
                .Mesh = resources.Meshes[m].get(),
                .TransformCB = m_ObjectConstantBuffers[i].get(),
                .Material = resources.Materials[model.Meshes[m].MaterialIndex],
                .ShadowCaster = entity.CastsShadow,
            });
        }
    }

    const SceneInfo sceneInfo = scene.GetSceneInfo();
    const LightData lightData = scene.GetLightData();

    frame.SceneInfoConstantBuffer->Write(&sceneInfo, sizeof(sceneInfo));
    frame.LightDataConstantBuffer->Write(&lightData, sizeof(lightData));

    ID3D12DescriptorHeap* heaps[] = {m_CbvSrvUavDescriptorHeap.Get()};
    cmdList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // Shadow map
    {
        if (m_ShadowMapState != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
            CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
                m_ShadowMapBuffer.Get(), m_ShadowMapState, D3D12_RESOURCE_STATE_DEPTH_WRITE)};
            cmdList->ResourceBarrier(1, &barrier);
            m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }

        cmdList->ClearDepthStencilView(shadowMapHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{0,    0,   static_cast<FLOAT>(s_ShadowMapSize), static_cast<FLOAT>(s_ShadowMapSize),
                                0.0f, 1.0f};
        D3D12_RECT scissorRect{0, 0, static_cast<UINT>(s_ShadowMapSize), static_cast<UINT>(s_ShadowMapSize)};
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);

        m_ShadowPass->OnRender(*frame.CommandList, shadowMapHandle, *frame.LightDataConstantBuffer, m_RenderItems);

        // Make the shadow map sampleable for the forward pass.
        CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
            m_ShadowMapBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)};
        cmdList->ResourceBarrier(1, &barrier);
        m_ShadowMapState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // Forward lighting pass
    {
        CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT,
                                                                              D3D12_RESOURCE_STATE_RENDER_TARGET)};

        cmdList->ResourceBarrier(1, &barrier);
        static constexpr FLOAT clearColor[]{0.4f, 0.6f, 0.9f, 1.0f};
        cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        cmdList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{
            0,    0,   static_cast<float>(m_SwapChain->GetWidth()), static_cast<float>(m_SwapChain->GetHeight()),
            0.0f, 1.0f};
        D3D12_RECT scissorRect{0, 0, static_cast<LONG>(m_SwapChain->GetWidth()),
                               static_cast<LONG>(m_SwapChain->GetHeight())};
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);

        m_ForwardLighting->OnRender(*frame.CommandList, rtvHandle, depthHandle, *frame.SceneInfoConstantBuffer,
                                    *frame.LightDataConstantBuffer, m_ShadowMapSRV, m_RenderItems);
    }

    m_RenderItems.clear();

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)};
        cmdList->ResourceBarrier(1, &barrier);

        frame.CommandList->Close();
        ID3D12CommandList* const ppCommandLists[]{cmdList};
        m_CommandQueue->ExecuteCommandLists(ppCommandLists);

        UINT syncInterval{m_VSync || !m_TearingSupported ? 1u : 0u};
        UINT presentFlags{!m_VSync && m_TearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0u};
        ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

        frame.FenceValue = m_Fence->Signal(m_CommandQueue->GetHandle());
    }
}

} // namespace GEngine
