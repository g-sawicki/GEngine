#include "PCH.hpp"

#include "EntityManager.hpp"

#include <utility>

namespace GEngine {

namespace {
// Shared fallback entity returned for satale/invalid handles
Entity g_InvalidEntity;
} // namespace

EntityId EntityManager::SpawnEntity(std::shared_ptr<const Model> model, const Transform& transform, bool castsShadow) {
    Entity entity;
    entity.Model = std::move(model);
    entity.Transform = transform;
    entity.CastsShadow = castsShadow;

    uint32_t slot{};
    uint32_t generation{};
    if (!m_FreeSlots.empty()) {
        slot = m_FreeSlots.back();
        m_FreeSlots.pop_back();
        generation = m_Generations[slot];
    } else {
        slot = static_cast<uint32_t>(m_Generations.size());
        m_Generations.push_back(1);
        m_SlotToDense.push_back(kInvalidSlot);
        generation = 1;
    }

    const uint32_t denseIndex = static_cast<uint32_t>(m_Entities.size());
    m_Entities.push_back(std::move(entity));
    m_DenseToSlot.push_back(slot);
    m_SlotToDense[slot] = denseIndex;

    return EntityId{.Index = slot, .Generation = generation};
}

Entity& EntityManager::GetEntity(EntityId id) {
    if (!IsValid(id))
        return g_InvalidEntity;
    return m_Entities[m_SlotToDense[id.Index]];
}

const Entity& EntityManager::GetEntity(EntityId id) const {
    if (!IsValid(id))
        return g_InvalidEntity;
    return m_Entities[m_SlotToDense[id.Index]];
}

bool EntityManager::IsValid(EntityId id) const noexcept {
    if (id.Index >= m_Generations.size())
        return false;
    if (m_Generations[id.Index] == 0)
        return false;
    if (id.Generation != m_Generations[id.Index])
        return false;
    return m_SlotToDense[id.Index] != kInvalidSlot;
}

void EntityManager::RemoveEntity(EntityId id) {
    if (!IsValid(id))
        return;

    const uint32_t slot = id.Index;
    const uint32_t denseIndex = m_SlotToDense[slot];

    // Swap the last entity into the hole to keep the dense array packed (O(1) remove).
    const uint32_t lastDenseIndex = static_cast<uint32_t>(m_Entities.size()) - 1;
    if (denseIndex != lastDenseIndex) {
        const uint32_t lastSlot = m_DenseToSlot[lastDenseIndex];
        m_Entities[denseIndex] = std::move(m_Entities[lastDenseIndex]);
        m_DenseToSlot[denseIndex] = lastSlot;
        m_SlotToDense[lastSlot] = denseIndex;
    }
    m_Entities.pop_back();
    m_DenseToSlot.pop_back();

    // Bump the generation so stale handles become detectable, and recycle the slot.
    ++m_Generations[slot];
    m_SlotToDense[slot] = kInvalidSlot;
    m_FreeSlots.push_back(slot);
}

} // namespace GEngine
