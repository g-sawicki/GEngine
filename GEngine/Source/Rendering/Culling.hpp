#pragma once

#include "Interop/Light.h"
#include "Rendering/Components.hpp"
#include "Rendering/Frustum.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace GEngine {

enum class CullFilter : uint8_t {
    All,
    ShadowCasters,
};

struct VisibleItems {
    std::vector<uint32_t> Indices;
};

struct CascadeVisibility {
    uint32_t Count{};
    std::array<VisibleItems, kMaxCascades> Cascades{};
};

void CullView(std::span<const RenderItem> items, const Frustum& frustum, VisibleItems& visibleItems,
              CullFilter filter = CullFilter::All);

void CullShadowCascades(std::span<const RenderItem> items, const CascadedShadowMapsData& cascades,
                        CascadeVisibility& cascadeVisibility);

} // namespace GEngine
