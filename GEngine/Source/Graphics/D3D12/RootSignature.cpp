#include "PCH.hpp"

#include "RootSignature.hpp"

#include "D3D12Common.hpp"

namespace GEngine {

using namespace Microsoft::WRL;

RootSignature::RootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC1& desc) {
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1 = desc,
    };
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&versionedDesc, &signature, &error));
    ThrowIfFailed(device.Get()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature)));
}

} // namespace GEngine
