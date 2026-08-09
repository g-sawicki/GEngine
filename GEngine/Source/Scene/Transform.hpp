#pragma once

#include <DirectXMath.h>

namespace GEngine {

struct Transform {
    DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4 Rotation{0.0f, 0.0f, 0.0f, 1.0f};
    DirectX::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};

    static Transform FromPosition(const DirectX::XMFLOAT3& position) noexcept {
        Transform transform{};
        transform.Position = position;
        return transform;
    }

    static Transform FromScale(const DirectX::XMFLOAT3& scale) noexcept {
        Transform transform{};
        transform.Scale = scale;
        return transform;
    }

    static Transform FromEulerDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept {
        Transform transform{};
        DirectX::XMStoreFloat4(&transform.Rotation,
                               DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(pitchDegrees),
                                                                         DirectX::XMConvertToRadians(yawDegrees),
                                                                         DirectX::XMConvertToRadians(rollDegrees)));
        return transform;
    }

    [[nodiscard]] const DirectX::XMFLOAT4X4& GetMatrix() const noexcept {
        const DirectX::XMMATRIX matrix =
            DirectX::XMMatrixAffineTransformation(DirectX::XMLoadFloat3(&Scale), DirectX::XMVectorZero(),
                                                  DirectX::XMLoadFloat4(&Rotation), DirectX::XMLoadFloat3(&Position));
        DirectX::XMStoreFloat4x4(&m_Matrix, matrix);
        return m_Matrix;
    }

    mutable DirectX::XMFLOAT4X4 m_Matrix{};
};

} // namespace GEngine
