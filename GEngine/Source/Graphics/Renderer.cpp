#include "PCH.hpp"

#include "Graphics/Renderer.hpp"

#include "Graphics/D3D12/Common.hpp"
#include "Rendering/MeshFactory.hpp"
#include "default_ps.h"
#include "default_vs.h"

using namespace Microsoft::WRL;

namespace GEngine {

static ComPtr<IDXGIAdapter4> GetAdapter(IDXGIFactory6* dxgiFactory, bool useWarp) {
    if (useWarp) {
        ComPtr<IDXGIAdapter4> result;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&result)));
        return result;
    }

    // DXGI 1.6+: prefer the high-performance discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (SUCCEEDED(
            dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)))) {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))) {
            ComPtr<IDXGIAdapter4> result;
            ThrowIfFailed(adapter.As(&result));
            return result;
        }
    }

    // Fallback: manually enumerate and pick the adapter with the most video memory
    SIZE_T maxDedicatedVideoMemory{};
    ComPtr<IDXGIAdapter4> selectedAdapter;
    for (UINT i{}; dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
            desc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
            maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
            ThrowIfFailed(adapter.As(&selectedAdapter));
        }
    }

    return selectedAdapter;
}

static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                         uint32_t numDescriptors) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type = type,
        .NumDescriptors = numDescriptors,
    };

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Renderer::Init(HWND hwnd, uint32_t width, uint32_t height, bool useWarp) {
    Device::EnableDebugLayer();

    ComPtr<IDXGIFactory6> dxgiFactory;
    UINT createFactoryFlags{};
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    m_TearingSupported = SwapChain::CheckTearingSupport(dxgiFactory.Get());

    ComPtr<IDXGIAdapter4> dxgiAdapter4{GetAdapter(dxgiFactory.Get(), useWarp)};

    m_Device = std::make_unique<Device>(dxgiAdapter4.Get());
    m_CommandQueue = std::make_unique<CommandQueue>(*m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        std::make_unique<SwapChain>(dxgiFactory.Get(), hwnd, *m_CommandQueue, width, height, SwapChain::NumFrames);

    m_RTVDescriptorHeap = CreateDescriptorHeap(m_Device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChain::NumFrames);
    m_RTVDescriptorSize = m_Device->Get()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews();

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ThrowIfFailed(m_Device->Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              IID_PPV_ARGS(&m_FrameResources[i].CommandAllocator)));
        m_FrameResources[i].CommandList = std::make_unique<CommandList>(
            *m_Device, m_FrameResources[i].CommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    m_Fence = std::make_unique<Fence>(*m_Device);

    CD3DX12_ROOT_PARAMETER rootParams[1];
    rootParams[0].InitAsConstantBufferView(0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(std::size(rootParams));
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    m_RootSignature = std::make_unique<RootSignature>(*m_Device, rootSigDesc);

    // Pipeline state object
    {
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
            .pRootSignature = m_RootSignature->Get(),
            .VS = {g_VSMain, sizeof(g_VSMain)},
            .PS = {g_PSMain, sizeof(g_PSMain)},
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))},
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = {SwapChain::BackBufferFormat},
            .SampleDesc = {.Count = 1, .Quality = 0},
        };

        m_PipelineState = std::make_unique<PipelineState>(*m_Device, psoDesc);
    }

    const Mesh& mesh{MeshFactory::Cube()};
    m_VertexStride = sizeof(mesh.vertices[0]);
    m_VertexBuffer = std::make_unique<Buffer>(*m_Device, mesh.vertices.size() * m_VertexStride, mesh.vertices.data());
    m_IndexBuffer =
        std::make_unique<Buffer>(*m_Device, mesh.indices.size() * sizeof(mesh.indices[0]), mesh.indices.data());
    m_IndexCount = static_cast<UINT>(mesh.indices.size());

    m_CameraConstantBuffer = std::make_unique<Buffer>(*m_Device, 256); // 64B ViewProjection + padding
}

void Renderer::Destroy() {
    if (m_Fence && m_CommandQueue) {
        m_Fence->Flush(m_CommandQueue->GetHandle());
    }

    // Reset in reverse order of creation.
    for (auto& frame : m_FrameResources) {
        frame.CommandList.reset();
        frame.CommandAllocator.Reset();
    }
    m_Fence.reset();
    m_SwapChain.reset();
    m_CommandQueue.reset();
    m_RTVDescriptorHeap.Reset();
    m_PipelineState.reset();
    m_RootSignature.reset();
    m_VertexBuffer.reset();
    m_IndexBuffer.reset();
    m_CameraConstantBuffer.reset();
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

void Renderer::UpdateRenderTargetViews() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

    for (uint32_t i{}; i < SwapChain::NumFrames; ++i) {
        ID3D12Resource* backBuffer{m_SwapChain->GetBackBuffer(i)};
        m_Device->Get()->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
        rtvHandle.Offset(m_RTVDescriptorSize);
    }
}

void Renderer::OnResize(uint32_t width, uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);

    m_Fence->Flush(m_CommandQueue->GetHandle());
    m_SwapChain->OnResize(width, height);

    UpdateRenderTargetViews();
}

void Renderer::Render(const ViewInfo& viewInfo) {
    auto currentIdx{m_SwapChain->GetCurrentBackBufferIndex()};
    auto& frame{m_FrameResources[currentIdx]};

    // Ensure the GPU has finished with this frame's resources before reusing them.
    m_Fence->WaitForValue(frame.FenceValue);

    // Upload the view-projection matrix.
    m_CameraConstantBuffer->Write(&viewInfo.ViewProjection, sizeof(viewInfo.ViewProjection));

    ID3D12Resource* backBuffer{m_SwapChain->GetCurrentBackBuffer()};

    frame.CommandList->Reset(frame.CommandAllocator.Get());
    auto* cmdList{frame.CommandList->GetHandle()};

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentIdx,
                                      m_RTVDescriptorSize);

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT,
                                                                              D3D12_RESOURCE_STATE_RENDER_TARGET)};

        cmdList->ResourceBarrier(1, &barrier);
        static constexpr FLOAT clearColor[]{0.4f, 0.6f, 0.9f, 1.0f};
        cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Draw
    {
        D3D12_VIEWPORT viewport{
            0,    0,   static_cast<float>(m_SwapChain->GetWidth()), static_cast<float>(m_SwapChain->GetHeight()),
            0.0f, 1.0f};
        D3D12_RECT scissorRect{0, 0, static_cast<LONG>(m_SwapChain->GetWidth()),
                               static_cast<LONG>(m_SwapChain->GetHeight())};

        cmdList->SetGraphicsRootSignature(m_RootSignature->Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_CameraConstantBuffer->GetGPUVirtualAddress());
        cmdList->SetPipelineState(m_PipelineState->Get());
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        auto vbv{m_VertexBuffer->GetVBV(m_VertexStride)};
        cmdList->IASetVertexBuffers(0, 1, &vbv);
        auto ibv{m_IndexBuffer->GetIBV()};
        cmdList->IASetIndexBuffer(&ibv);

        cmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
    }

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
