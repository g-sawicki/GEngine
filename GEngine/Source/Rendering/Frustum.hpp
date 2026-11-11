#pragma once

#include <DirectXCollision.h>
#include <DirectXMath.h>

namespace GEngine {

struct Frustum {
    [[nodiscard]] static Frustum FromViewProjection(const DirectX::FXMMATRIX viewProjection) noexcept {
        const DirectX::XMMATRIX m = DirectX::XMMatrixTranspose(viewProjection);
        Frustum frustum;
        frustum.Planes[0] = DirectX::XMVectorAdd(m.r[3], m.r[0]);      // left
        frustum.Planes[1] = DirectX::XMVectorSubtract(m.r[3], m.r[0]); // right
        frustum.Planes[2] = DirectX::XMVectorAdd(m.r[3], m.r[1]);      // bottom
        frustum.Planes[3] = DirectX::XMVectorSubtract(m.r[3], m.r[1]); // top
        frustum.Planes[4] = m.r[2];                                    // near
        frustum.Planes[5] = DirectX::XMVectorSubtract(m.r[3], m.r[2]); // far
        return frustum;
    }

    [[nodiscard]] bool Intersects(const DirectX::BoundingBox& box) const noexcept {
        const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&box.Center);
        const DirectX::XMVECTOR extents = DirectX::XMLoadFloat3(&box.Extents);

        for (const DirectX::XMVECTOR plane : Planes) {
            const DirectX::XMVECTOR absNormal = DirectX::XMVectorAbs(plane);
            const float radius = DirectX::XMVectorGetX(DirectX::XMVector3Dot(absNormal, extents));
            const float signedDistance =
                DirectX::XMVectorGetX(DirectX::XMVector3Dot(plane, center)) + DirectX::XMVectorGetW(plane);
            if (signedDistance + radius < 0.0f)
                return false;
        }
        return true;
    }

    DirectX::XMVECTOR Planes[6]{};
};

} // namespace GEngine
