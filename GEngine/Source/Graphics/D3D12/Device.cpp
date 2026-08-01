#include "PCH.hpp"

#include "Device.hpp"

#include "Common.hpp"

using namespace Microsoft::WRL;

namespace GEngine {

void Device::EnableDebugLayer() {
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();

    // Enable GPU-Based Validation
    ComPtr<ID3D12Debug3> debug3;
    if (SUCCEEDED(debugInterface.As(&debug3))) {
        debug3->SetEnableGPUBasedValidation(TRUE);
    }

    // Enable DRED auto-breadcrumbs and page-fault reporting
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)))) {
        dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    }
#endif
}

ComPtr<IDXGIFactory6> Device::CreateDXGIFactory() {
    ComPtr<IDXGIFactory6> dxgiFactory;
    UINT createFactoryFlags{};
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    return dxgiFactory;
}

ComPtr<IDXGIAdapter4> Device::GetAdapter(IDXGIFactory6* dxgiFactory, bool useWarp) {
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

Device::Device(IDXGIAdapter4* adapter) {
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(m_Device.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        D3D12_INFO_QUEUE_FILTER newFilter{};

        static D3D12_MESSAGE_SEVERITY denySeverities[] = {
            D3D12_MESSAGE_SEVERITY_INFO,
        };
        newFilter.DenyList.NumSeverities = static_cast<UINT>(std::size(denySeverities));
        newFilter.DenyList.pSeverityList = denySeverities;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
    }
#endif
}

bool Device::IsDeviceRemoved() const noexcept {
    const HRESULT hr{m_Device->GetDeviceRemovedReason()};
    if (FAILED(hr)) {
        wchar_t buf[64];
        swprintf_s(buf, L"Device removed: 0x%08X\n", static_cast<unsigned>(hr));
        ::OutputDebugStringW(buf);
        return true;
    }
    return false;
}

} // namespace GEngine
