#pragma once

#include "engine/ecs/entity.hpp"
#include "engine/core/types.hpp"

#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <cassert>
#include <functional>

namespace mf {

// ---------------------------------------------------------------------------
// Sparse component storage (simple, not the final high-perf version)
// ---------------------------------------------------------------------------

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID id) = 0;
    virtual bool has(EntityID id) const = 0;
};

template <typename T>
class ComponentPool : public IComponentPool {
public:
    void emplace(EntityID id, T component) {
        data_[id] = std::move(component);
    }

    T* get(EntityID id) {
        auto it = data_.find(id);
        return it != data_.end() ? &it->second : nullptr;
    }

    const T* get(EntityID id) const {
        auto it = data_.find(id);
        return it != data_.end() ? &it->second : nullptr;
    }

    void remove(EntityID id) override {
        data_.erase(id);
    }

    bool has(EntityID id) const override {
        return data_.count(id) > 0;
    }

    auto begin() { return data_.begin(); }
    auto end()   { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end()   const { return data_.end(); }

private:
    std::unordered_map<EntityID, T> data_;
};

// ---------------------------------------------------------------------------
// Entity Registry — owns entities and component pools
// ---------------------------------------------------------------------------

class EntityRegistry {
public:
    EntityRegistry() = default;

    EntityID spawn() {
        EntityID id = next_id_++;
        entities_[id] = Entity{id, {}, true};
        return id;
    }

    void destroy(EntityID id) {
        auto it = entities_.find(id);
        if (it == entities_.end() || !it->second.alive) return;

        // Remove from all pools
        for (auto& [type, pool] : pools_) {
            pool->remove(id);
        }
        it->second.alive = false;
        it->second.components.reset();
    }

    bool alive(EntityID id) const {
        auto it = entities_.find(id);
        return it != entities_.end() && it->second.alive;
    }

    template <typename T>
    void add(EntityID id, T component) {
        assert(alive(id));
        auto& pool = get_or_create_pool<T>();
        pool.emplace(id, std::move(component));

        // Mark bit (we assign type indices lazily)
        size_t bit = type_index<T>();
        entities_[id].components.set(bit);
    }

    template <typename T>
    T* get(EntityID id) {
        auto* pool = try_get_pool<T>();
        return pool ? pool->get(id) : nullptr;
    }

    template <typename T>
    const T* get(EntityID id) const {
        auto* pool = try_get_pool<T>();
        return pool ? pool->get(id) : nullptr;
    }

    template <typename T>
    bool has(EntityID id) const {
        auto* pool = try_get_pool<T>();
        return pool && pool->has(id);
    }

    template <typename T>
    void remove(EntityID id) {
        auto* pool = try_get_pool<T>();
        if (pool) {
            pool->remove(id);
            entities_[id].components.reset(type_index<T>());
        }
    }

    // Iterate all living entities that possess every component in the pack
    template <typename... Components, typename Fn>
    void each(Fn&& fn) {
        for (auto& [id, entity] : entities_) {
            if (!entity.alive) continue;
            if ((has<Components>(id) && ...)) {
                fn(id, *get<Components>(id)...);
            }
        }
    }

    size_t living_count() const {
        size_t n = 0;
        for (const auto& [_, e] : entities_) if (e.alive) ++n;
        return n;
    }

private:
    EntityID next_id_ = 1; // 0 is INVALID
    std::unordered_map<EntityID, Entity> entities_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
    std::unordered_map<std::type_index, size_t> type_bits_;
    size_t next_type_bit_ = 0;

    template <typename T>
    size_t type_index() {
        std::type_index ti(typeid(T));
        auto it = type_bits_.find(ti);
        if (it != type_bits_.end()) return it->second;
        assert(next_type_bit_ < MAX_COMPONENTS);
        size_t bit = next_type_bit_++;
        type_bits_[ti] = bit;
        return bit;
    }

    template <typename T>
    ComponentPool<T>& get_or_create_pool() {
        std::type_index ti(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            auto* raw = pool.get();
            pools_[ti] = std::move(pool);
            return *raw;
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T>
    ComponentPool<T>* try_get_pool() {
        std::type_index ti(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end()) return nullptr;
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T>
    const ComponentPool<T>* try_get_pool() const {
        std::type_index ti(typeid(T));
        auto it = pools_.find(ti);
        if (it == pools_.end()) return nullptr;
        return static_cast<const ComponentPool<T>*>(it->second.get());
    }
};

} // namespace mf
