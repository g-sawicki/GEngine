#include "PCH.hpp"

#include "RootSignature.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

RootSignature::RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC& desc) {
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(device.Get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature)));
}

} // namespace GEngine
