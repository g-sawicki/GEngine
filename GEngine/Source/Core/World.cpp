#include "PCH.hpp"

#include "World.hpp"

namespace GEngine {

Camera& World::CreateCamera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position) {
    m_ActiveCamera = std::make_unique<Camera>(desc, position);
    return *m_ActiveCamera;
}

} // namespace GEngine
