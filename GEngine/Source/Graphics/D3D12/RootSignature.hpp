#pragma once

#include "Device.hpp"

namespace GEngine {

class RootSignature {
  public:
    RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC& desc);

    [[nodiscard]] ID3D12RootSignature* Get() const noexcept { return m_RootSignature.Get(); }

  private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
};

} // namespace GEngine
