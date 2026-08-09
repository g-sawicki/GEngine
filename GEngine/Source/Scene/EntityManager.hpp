#pragma once

#include "Scene/Entity.hpp"
#include "Scene/Transform.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace GEngine {

class EntityManager {
  public:
    EntityManager() = default;

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = delete;
    EntityManager& operator=(EntityManager&&) = delete;

    EntityId SpawnEntity(std::shared_ptr<const Model> model, const Transform& transform = {}, bool castsShadow = true);
    // Returns a reference to the live entity. A stale/invalid handle yields the shared dummy entity.
    [[nodiscard]] Entity& GetEntity(EntityId id);
    [[nodiscard]] const Entity& GetEntity(EntityId id) const;
    [[nodiscard]] bool IsValid(EntityId id) const noexcept;
    void RemoveEntity(EntityId id);

    // Dense render-order view of all live entities.
    [[nodiscard]] const std::vector<Entity>& GetEntities() const noexcept { return m_Entities; }

  private:
    static constexpr uint32_t kInvalidSlot{0xFFFFFFFF};

    std::vector<Entity> m_Entities;
    std::vector<uint32_t> m_DenseToSlot;
    std::vector<uint32_t> m_SlotToDense;
    std::vector<uint32_t> m_Generations;
    std::vector<uint32_t> m_FreeSlots;
};

} // namespace GEngine
