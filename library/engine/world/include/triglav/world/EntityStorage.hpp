#pragma once

#include "ComponentStorage.hpp"
#include "HierarchyTree.hpp"

#include <span>

namespace triglav::world {

class EntityStorage
{
 public:
   void init();
   [[nodiscard]] void* get_component(EntityID entity_id, ComponentID component_id) const;
   [[nodiscard]] bool get_components(EntityID entity_id, std::span<const ComponentID> component_ids, std::span<void*> out_components) const;
   void allocate_component(EntityID entity_id, ComponentID component_id);
   void allocate_components(EntityID entity_id, std::span<ComponentID> component_id);
   [[nodiscard]] EntityID allocate_entity(EntityID parent);
   [[nodiscard]] bool has_component(EntityID entity_id, ComponentID component_id) const;
   [[nodiscard]] ComponentStorage& component_storage(ComponentID component_id);
   [[nodiscard]] HierarchyTree& hierarchy_tree();
   [[nodiscard]] const HierarchyTree& hierarchy_tree() const;

   bool serialize(io::IWriter& writer) const;
   bool deserialize(io::IReader& reader);

 private:
   std::vector<ComponentStorage> m_storage;
   HierarchyTree m_hierarchy_tree;
   EntityID m_top_entity_id = 1;
};

}// namespace triglav::world
