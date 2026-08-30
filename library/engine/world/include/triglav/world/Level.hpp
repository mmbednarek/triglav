#pragma once

#include "ComponentManager.hpp"
#include "EntityStorage.hpp"
#include "LevelNode.hpp"
#include "World.hpp"

#include "triglav/meta/Meta.hpp"

#include <map>
#include <memory>
#include <set>

namespace triglav::io {
class Path;
}

namespace triglav::world {

template<typename TComp>
class LevelComponentIterator
{
 public:
   using iterator_category = std::forward_iterator_tag;
   using value_type = std::pair<EntityID, TComp&>;
   using difference_type = std::ptrdiff_t;
   using pointer = value_type*;
   using reference = value_type&;

   explicit LevelComponentIterator(ComponentIterator iterator) :
       m_iterator(iterator)
   {
   }

   LevelComponentIterator& operator++()
   {
      ++m_iterator;
      return *this;
   }

   [[nodiscard]] value_type operator*() const
   {
      auto [id, ptr] = *m_iterator;
      return {id, *static_cast<TComp*>(ptr)};
   }

   [[nodiscard]] bool operator!=(const LevelComponentIterator& other) const
   {
      return m_iterator != other.m_iterator;
   }

 private:
   ComponentIterator m_iterator;
};


template<typename TComp>
struct LevelComponentRange
{
   LevelComponentIterator<TComp> from;
   LevelComponentIterator<TComp> to;

   LevelComponentIterator<TComp> begin() const
   {
      return from;
   }

   LevelComponentIterator<TComp> end() const
   {
      return to;
   }
};

class Level
{
 public:
   void add_node(Name id, LevelNode&& node);

   void init_entities();

   LevelNode& at(Name id);
   LevelNode& root();

   [[nodiscard]] meta::Ref component_ref(EntityID entity_id, Name component_name) const;

   void serialize_yaml(c4::yml::NodeRef& node) const;
   [[nodiscard]] bool save_to_file(const io::Path& path) const;

   [[nodiscard]] EntityID new_entity(EntityID parent = ROOT_ENTITY);

   template<meta::HasMetaName T>
   bool has_component(const EntityID id)
   {
      return m_entity_storage.has_component(id, *ComponentManager::the().component_id_by_class_name(T::meta_name()));
   }

   template<meta::HasMetaName T>
   const T& component(const EntityID id) const
   {
      return *static_cast<T*>(m_entity_storage.get_component(id, *ComponentManager::the().component_id_by_class_name(T::meta_name())));
   }

   template<meta::HasMetaName T>
   T& mut_component(const EntityID id)
   {
      const auto component_id = *ComponentManager::the().component_id_by_class_name(T::meta_name());
      this->insert_component_change(component_id, id);
      return *static_cast<T*>(m_entity_storage.get_component(id, component_id));
   }

   template<meta::HasMetaName T>
   const T* component_opt(const EntityID id) const
   {
      const auto comp_id = ComponentManager::the().component_id_by_class_name(T::meta_name());
      if (!comp_id.has_value())
         return nullptr;

      return static_cast<T*>(m_entity_storage.get_component(id, *comp_id));
   }

   template<meta::HasMetaName... T>
   std::tuple<const T&...> components(const EntityID entity_id)
   {
      std::array<Name, sizeof...(T)> comp_names{T::meta_name()...};
      std::array<ComponentID, sizeof...(T)> comp_ids{};
      std::array<void*, sizeof...(T)> comp_ptrs{};

      assert(ComponentManager::the().class_names_to_ids(comp_names, comp_ids));
      assert(m_entity_storage.get_components(entity_id, comp_ids, comp_ptrs));

      return []<mem_size... Idx>(std::array<void*, sizeof...(T)>& ptrs, std::index_sequence<Idx...>) -> std::tuple<T&...> {
         return {*static_cast<T*>(ptrs[Idx])...};
      }(comp_ptrs, std::index_sequence_for<T...>{});
   }

   template<meta::HasMetaName T>
   T* insert_component(const EntityID entity_id)
   {
      const auto comp_id = ComponentManager::the().component_id_by_class_name(T::meta_name());
      if (!comp_id.has_value())
         return nullptr;

      m_entity_storage.allocate_component(entity_id, *comp_id);
      m_addition_lists[*comp_id].emplace_back(entity_id);
      return static_cast<T*>(m_entity_storage.get_component(entity_id, *comp_id));
   }

   template<meta::HasMetaName T>
   LevelComponentRange<T> all()
   {
      auto& storage = m_entity_storage.component_storage(*ComponentManager::the().component_id_by_class_name(T::meta_name()));
      auto from = LevelComponentIterator<T>(storage.begin());
      auto to = LevelComponentIterator<T>(storage.end());
      return LevelComponentRange<T>{from, to};
   }

   [[nodiscard]] HierarchyTree::Range children_of(EntityID entity) const;
   [[nodiscard]] EntityID parent_of(EntityID entity) const;
   void register_system(ISystem& system, std::span<Name> component_class_names);
   void flush();

 private:
   void insert_component_change(ComponentID id, EntityID entity);

   struct SystemRegistration
   {
      ISystem* system;
      std::set<ComponentID> component_ids;
   };

   struct ComponentChanges
   {
      ComponentID component_id;
      EntityID entity_id;
   };

   std::map<Name, LevelNode> m_nodes;
   EntityStorage m_entity_storage;
   std::vector<SystemRegistration> m_systems;
   std::map<ComponentID, std::vector<EntityID>> m_change_lists;
   std::map<ComponentID, std::vector<EntityID>> m_addition_lists;
};

}// namespace triglav::world