#pragma once

#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace GEngine {

using Entity = uint32_t;

template <typename Component>
class ComponentStorage {
  public:
    using iterator = typename std::unordered_map<Entity, Component>::iterator;
    using const_iterator = typename std::unordered_map<Entity, Component>::const_iterator;

    template <typename... Args>
    Component& AddComponent(Entity entity, Args&&... args) {
        auto [it, inserted]{m_Components.insert_or_assign(entity, Component{std::forward<Args>(args)...})};
        return it->second;
    }

    [[nodiscard]] Component* GetComponent(Entity entity) {
        auto it = m_Components.find(entity);
        return (it != m_Components.end()) ? &it->second : nullptr;
    }

    [[nodiscard]] const Component* GetComponent(Entity entity) const {
        auto it = m_Components.find(entity);
        return (it != m_Components.end()) ? &it->second : nullptr;
    }

    iterator begin() noexcept { return m_Components.begin(); }
    iterator end() noexcept { return m_Components.end(); }
    const_iterator begin() const noexcept { return m_Components.begin(); }
    const_iterator end() const noexcept { return m_Components.end(); }

  private:
    // PERF: Poor cache locality. Look into sparse sets.
    std::unordered_map<Entity, Component> m_Components;
};

template <typename Storage>
class ComponentView {
  public:
    explicit ComponentView(Storage& storage) : m_Storage(storage) {}

    auto begin() noexcept { return m_Storage.begin(); }
    auto end() noexcept { return m_Storage.end(); }
    auto begin() const noexcept { return m_Storage.begin(); }
    auto end() const noexcept { return m_Storage.end(); }

  private:
    Storage& m_Storage;
};

template <typename... Components>
class EntityRegistry {
  public:
    EntityRegistry() = default;

    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator=(const EntityRegistry&) = delete;
    EntityRegistry(EntityRegistry&&) = delete;
    EntityRegistry& operator=(EntityRegistry&&) = delete;

    [[nodiscard]] Entity Create() noexcept { return m_EntityIndex++; }

    template <typename Component, typename... Args>
    Component& AddComponent(Entity entity, Args&&... args) {
        return GetStorage<Component>().AddComponent(entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    [[nodiscard]] Component* GetComponent(Entity entity) noexcept {
        return GetStorage<Component>().GetComponent(entity);
    }

    template <typename Component>
    [[nodiscard]] const Component* GetComponent(Entity entity) const noexcept {
        return GetStorage<Component>().GetComponent(entity);
    }

    template <typename Component>
    [[nodiscard]] auto View() noexcept {
        return ComponentView<ComponentStorage<Component>>{GetStorage<Component>()};
    }

    template <typename Component>
    [[nodiscard]] auto View() const noexcept {
        return ComponentView<const ComponentStorage<Component>>{GetStorage<Component>()};
    }

  private:
    template <typename Component>
    [[nodiscard]] ComponentStorage<Component>& GetStorage() noexcept {
        return std::get<ComponentStorage<Component>>(m_Storages);
    }

    template <typename Component>
    [[nodiscard]] const ComponentStorage<Component>& GetStorage() const noexcept {
        return std::get<ComponentStorage<Component>>(m_Storages);
    }

    Entity m_EntityIndex{};
    std::tuple<ComponentStorage<Components>...> m_Storages;
};

} // namespace GEngine
