#pragma once

#include "Graphics/D3D12/Buffer.hpp"

#include <DirectXMath.h>
#include <d3d12.h>

#include <cstdint>
#include <vector>

namespace GEngine {

struct Transform {
    DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 Rotation{0.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] DirectX::XMMATRIX GetMatrix() const noexcept {
        return DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat3(&Scale), DirectX::XMVectorZero(),
                                                     DirectX::XMLoadFloat4(&Rotation),
                                                     DirectX::XMLoadFloat3(&Position));
    }
};

class MeshBuffer;

struct MaterialGPU {
    uint32_t AlbedoIndex{};
    uint32_t NormalIndex{};
    uint32_t RoughnessMetallicIndex{};
};

struct RenderItem {
    const MeshBuffer* Mesh{};
    const Buffer* TransformCB{};
    MaterialGPU Material{};
    bool ShadowCaster{true};
};

struct ModelHandle {
    uint32_t Id{};

    bool IsValid() const { return Id != 0; }
    bool operator==(const ModelHandle& other) const { return Id == other.Id; }
};

struct ModelComponent {
    ModelHandle Model{};
    bool CastsShadow{true};
};

} // namespace GEngine
