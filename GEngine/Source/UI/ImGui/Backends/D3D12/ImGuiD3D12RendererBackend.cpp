#include "PCH.hpp"

#include "ImGuiD3D12RendererBackend.hpp"

#include <backends/imgui_impl_dx12.h>

namespace GEngine::UI {

ImGuiD3D12RendererBackend::ImGuiD3D12RendererBackend(Device& device, CommandQueue& commandQueue,
                                                     DescriptorHeap& srvHeap, uint32_t framesInFlight,
                                                     DXGI_FORMAT renderTargetFormat)
    : m_Device(device), m_CommandQueue(commandQueue), m_SrvHeap(srvHeap), m_FramesInFlight(framesInFlight),
      m_RenderTargetFormat(renderTargetFormat) {}

ImGuiD3D12RendererBackend::~ImGuiD3D12RendererBackend() {
    if (m_Initialized)
        ImGui_ImplDX12_Shutdown();
}

void ImGuiD3D12RendererBackend::Initialize() {
    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = m_Device.Get();
    initInfo.CommandQueue = m_CommandQueue.GetHandle();
    initInfo.NumFramesInFlight = static_cast<int>(m_FramesInFlight);
    initInfo.RTVFormat = m_RenderTargetFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = m_SrvHeap.Get();
    initInfo.UserData = &m_SrvHeap;
    initInfo.SrvDescriptorAllocFn = &AllocateDescriptor;
    initInfo.SrvDescriptorFreeFn = &FreeDescriptor;

    if (!ImGui_ImplDX12_Init(&initInfo))
        throw std::runtime_error("Failed to initialize the ImGui D3D12 backend.");
    m_Initialized = true;
}

void ImGuiD3D12RendererBackend::NewFrame() {
    ImGui_ImplDX12_NewFrame();
}

void ImGuiD3D12RendererBackend::Render(ImDrawData* drawData, void* nativeCommandList) {
    assert(nativeCommandList);
    ImGui_ImplDX12_RenderDrawData(drawData, static_cast<ID3D12GraphicsCommandList*>(nativeCommandList));
}

void ImGuiD3D12RendererBackend::AllocateDescriptor(ImGui_ImplDX12_InitInfo* info,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
                                                   D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle) {
    auto& srvHeap = *static_cast<DescriptorHeap*>(info->UserData);
    const uint32_t index = srvHeap.Allocate();
    *cpuHandle = srvHeap.GetCpuHandle(index);
    *gpuHandle = srvHeap.GetGpuHandle(index);
}

void ImGuiD3D12RendererBackend::FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
                                               [[maybe_unused]] D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
    auto& srvHeap = *static_cast<DescriptorHeap*>(info->UserData);
    srvHeap.Deallocate(srvHeap.GetIndex(cpuHandle));
}

} // namespace GEngine::UI
