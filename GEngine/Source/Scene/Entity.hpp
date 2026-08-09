#pragma once

#include "Scene/Material.hpp"
#include "Scene/Model.hpp"
#include "Scene/Transform.hpp"

#include <cstdint>
#include <memory>

namespace GEngine {

struct EntityId {
    uint32_t Index{};
    uint32_t Generation{};

    bool operator==(const EntityId&) const = default;

    [[nodiscard]] bool IsNull() const noexcept { return Index == 0 && Generation == 0; }
};

struct Entity {
    std::shared_ptr<const Model> Model;
    Transform Transform;
    bool CastsShadow{true};
};

} // namespace GEngine
