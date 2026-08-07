#include "PCH.hpp"

#include "Graphics/Renderer.hpp"

#include "Graphics/D3D12/Common.hpp"
#include "Rendering/MeshBuffer.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

static ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors,
                     D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type = type,
        .NumDescriptors = numDescriptors,
        .Flags = flags,
    };

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Renderer::Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp) {
    Device::EnableDebugLayer();
    ComPtr<IDXGIFactory6> dxgiFactory = Device::CreateDXGIFactory();
    ComPtr<IDXGIAdapter4> dxgiAdapter4{Device::GetAdapter(dxgiFactory.Get(), useWarp)};
    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        std::make_unique<SwapChain>(dxgiFactory.Get(), hwnd, *m_CommandQueue, width, height, SwapChain::NumFrames);

    m_RTVDescriptorHeap = CreateDescriptorHeap(m_Device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChain::NumFrames);
    m_RTVDescriptorSize = m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_DSVDescriptorHeap = CreateDescriptorHeap(m_Device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2);
    m_DSVDescriptorSize = m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_TextureSRVHeap = CreateDescriptorHeap(m_Device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxTextures + 1,
                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    m_TextureSRVDescriptorSize =
        m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_DepthStencilBuffer = CreateDepthBuffer(width, height);
    m_ShadowMapBuffer = CreateDepthBuffer(s_ShadowMapSize, s_ShadowMapSize);

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1,
                                            m_DSVDescriptorSize};

    D3D12_DEPTH_STENCIL_VIEW_DESC shadowDsvDesc{
        .Format = s_DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {.MipSlice = 0},
    };
    m_Device->Get()->CreateDepthStencilView(m_ShadowMapBuffer.Get(), &shadowDsvDesc, dsvHandle);

    CreateShadowMapSRV();

    UpdateRenderTargetViews();

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ThrowIfFailed(m_Device->Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              IID_PPV_ARGS(&m_FrameResources[i].CommandAllocator)));
        m_FrameResources[i].CommandList = std::make_unique<CommandList>(
            *m_Device, m_FrameResources[i].CommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        static constexpr UINT64 kSceneInfoCBSize =
            (sizeof(SceneInfo) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) &
            ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ULL);
        m_FrameResources[i].SceneInfoConstantBuffer = std::make_unique<Buffer>(*m_Device, kSceneInfoCBSize);

        static constexpr UINT64 kLightDataCBSize =
            (sizeof(LightData) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) &
            ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ULL);
        m_FrameResources[i].LightDataConstantBuffer = std::make_unique<Buffer>(*m_Device, kLightDataCBSize);
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
    m_RTVDescriptorHeap.Reset();
    m_DepthStencilBuffer.Reset();
    m_ShadowMapBuffer.Reset();
    m_DSVDescriptorHeap.Reset();
    m_ForwardLighting.reset();
    m_ShadowPass.reset();
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

std::unique_ptr<MeshBuffer> Renderer::CreateMeshBuffer(const Mesh& mesh) {
    return std::make_unique<MeshBuffer>(*m_Device, mesh);
}

std::unique_ptr<Buffer> Renderer::CreateConstantBuffer(UINT64 size) {
    static constexpr UINT64 kAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    const UINT64 alignedSize = (size + kAlignment - 1) & ~(kAlignment - 1ULL);
    return std::make_unique<Buffer>(*m_Device, alignedSize);
}

void Renderer::DrawMesh(const MeshBuffer& mesh, const Buffer& objectCB, D3D12_GPU_DESCRIPTOR_HANDLE diffuseSRV) {
    m_RenderItems.push_back(
        {.Mesh = &mesh, .TransformCB = &objectCB, .Material = {.DiffuseSRV = diffuseSRV, .SpecularSRV = {}}});
}

std::unique_ptr<Texture> Renderer::CreateTexture(const Image& image) {
    if (m_NextTextureSRVIndex >= kMaxTextures)
        throw std::runtime_error("Exceeded maximum number of textures.");
    return std::make_unique<Texture>(*m_Device, *m_CommandQueue, m_TextureSRVHeap.Get(), m_TextureSRVDescriptorSize,
                                     m_NextTextureSRVIndex++, image);
}

void Renderer::UpdateRenderTargetViews() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ID3D12Resource* backBuffer{m_SwapChain->GetBackBuffer(i)};
        m_Device->Get()->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
        rtvHandle.Offset(m_RTVDescriptorSize);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        .Format = s_DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {.MipSlice = 0},
    };
    m_Device->Get()->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, dsvHandle);
}

void Renderer::CreateShadowMapSRV() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle{m_TextureSRVHeap->GetCPUDescriptorHandleForHeapStart(),
                                            static_cast<INT>(kShadowMapSRVIndex), m_TextureSRVDescriptorSize};

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = DXGI_FORMAT_R32_FLOAT,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = 1},
    };
    m_Device->Get()->CreateShaderResourceView(m_ShadowMapBuffer.Get(), &srvDesc, cpuHandle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle{m_TextureSRVHeap->GetGPUDescriptorHandleForHeapStart(),
                                            static_cast<INT>(kShadowMapSRVIndex), m_TextureSRVDescriptorSize};
    m_ShadowMapSRV = gpuHandle;
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

void Renderer::Render(const SceneInfo& sceneInfo, const LightData& lightData) {
    auto currentIdx{m_SwapChain->GetCurrentBackBufferIndex()};
    auto& frame{m_FrameResources[currentIdx]};

    // Ensure the GPU has finished with this frame's resources before reusing them.
    m_Fence->WaitForValue(frame.FenceValue);

    ID3D12Resource* backBuffer{m_SwapChain->GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList{frame.CommandList->GetHandle()};

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentIdx,
                                      m_RTVDescriptorSize);

    CD3DX12_CPU_DESCRIPTOR_HANDLE depthDsv(m_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE shadowMapDSV(depthDsv, 1, m_DSVDescriptorSize);

    frame.SceneInfoConstantBuffer->Write(&sceneInfo, sizeof(sceneInfo));
    frame.LightDataConstantBuffer->Write(&lightData, sizeof(lightData));

    ID3D12DescriptorHeap* heaps[] = {m_TextureSRVHeap.Get()};
    cmdList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // Shadow map
    {
        if (m_ShadowMapState != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
            CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
                m_ShadowMapBuffer.Get(), m_ShadowMapState, D3D12_RESOURCE_STATE_DEPTH_WRITE)};
            cmdList->ResourceBarrier(1, &barrier);
            m_ShadowMapState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }

        cmdList->ClearDepthStencilView(shadowMapDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{0,    0,   static_cast<FLOAT>(s_ShadowMapSize), static_cast<FLOAT>(s_ShadowMapSize),
                                0.0f, 1.0f};
        D3D12_RECT scissorRect{0, 0, static_cast<UINT>(s_ShadowMapSize), static_cast<UINT>(s_ShadowMapSize)};
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);

        m_ShadowPass->OnRender(*frame.CommandList, shadowMapDSV, *frame.LightDataConstantBuffer, m_RenderItems);

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
        cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        cmdList->ClearDepthStencilView(depthDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{
            0,    0,   static_cast<float>(m_SwapChain->GetWidth()), static_cast<float>(m_SwapChain->GetHeight()),
            0.0f, 1.0f};
        D3D12_RECT scissorRect{0, 0, static_cast<LONG>(m_SwapChain->GetWidth()),
                               static_cast<LONG>(m_SwapChain->GetHeight())};
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);

        m_ForwardLighting->OnRender(*frame.CommandList, rtv, depthDsv, *frame.SceneInfoConstantBuffer,
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
