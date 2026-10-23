#pragma once

#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/DescriptorHeap.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "UI/ImGui/ImGuiBackend.hpp"

#include <memory>

struct ImGui_ImplDX12_InitInfo;

namespace GEngine::UI {

class ImGuiD3D12RendererBackend final : public ImGuiRendererBackend {
  public:
    ImGuiD3D12RendererBackend(Device& device, CommandQueue& commandQueue, DescriptorHeap& srvHeap,
                              uint32_t framesInFlight, DXGI_FORMAT renderTargetFormat);
    ~ImGuiD3D12RendererBackend() override;

    void Initialize() override;
    void NewFrame() override;
    void Render(ImDrawData* drawData, void* nativeCommandList) override;

  private:
    static void AllocateDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
                                   D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);
    static void FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
                               D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    Device& m_Device;
    CommandQueue& m_CommandQueue;
    DescriptorHeap& m_SrvHeap;
    uint32_t m_FramesInFlight{};
    DXGI_FORMAT m_RenderTargetFormat{DXGI_FORMAT_UNKNOWN};
    bool m_Initialized{};
};

} // namespace GEngine::UI
