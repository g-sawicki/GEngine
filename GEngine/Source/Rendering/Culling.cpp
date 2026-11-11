#include "PCH.hpp"

#include "Rendering/Culling.hpp"

#include <algorithm>

namespace GEngine {

void CullView(std::span<const RenderItem> items, const Frustum& frustum, VisibleItems& visibleItems,
              const CullFilter filter) {
    visibleItems.Indices.clear();
    visibleItems.Indices.reserve(items.size());

    const bool includeNonCasters = filter == CullFilter::All;
    const auto isVisible = [&frustum, includeNonCasters](const RenderItem& item) {
        return (includeNonCasters || item.ShadowCaster) && frustum.Intersects(item.BoundingBox);
    };

    const auto itemCount = static_cast<uint32_t>(items.size());
    for (uint32_t itemIndex{}; itemIndex < itemCount; ++itemIndex) {
        if (isVisible(items[itemIndex]))
            visibleItems.Indices.push_back(itemIndex);
    }
}

void CullShadowCascades(std::span<const RenderItem> items, const CascadedShadowMapsData& cascades,
                        CascadeVisibility& cascadeVisibility) {
    cascadeVisibility.Count = std::min(cascades.cascadeCount, kMaxCascades);
    for (uint32_t cascadeIndex{}; cascadeIndex < cascadeVisibility.Count; ++cascadeIndex) {
        const Frustum frustum =
            Frustum::FromViewProjection(DirectX::XMLoadFloat4x4(&cascades.lightViewProjection[cascadeIndex]));
        CullView(items, frustum, cascadeVisibility.Cascades[cascadeIndex], CullFilter::ShadowCasters);
    }
}

} // namespace GEngine
