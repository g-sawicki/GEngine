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

Device::Device(IDXGIAdapter4* adapter) {
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(m_Device.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

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
