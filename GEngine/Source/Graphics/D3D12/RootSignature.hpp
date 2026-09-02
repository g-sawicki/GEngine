#pragma once

#include "Core/Utility/Defines.hpp"
#include "Device.hpp"

namespace GEngine {

class RootSignature {
  public:
    RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC1& desc);

    GE_NO_COPY(RootSignature)
    GE_DEFAULT_MOVE(RootSignature)

    [[nodiscard]] ID3D12RootSignature* Get() const noexcept { return m_RootSignature.Get(); }

  private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
};

} // namespace GEngine
