#include "EntityStorage.hpp"

#include "ComponentManager.hpp"
#include "triglav/io/Serializer.hpp"

namespace triglav::world {

void EntityStorage::init()
{
   m_storage.reserve(ComponentManager::the().count());
   std::generate_n(std::back_inserter(m_storage), ComponentManager::the().count(), [i = 0]() mutable { return ComponentStorage(i++); });
}

void* EntityStorage::get_component(const EntityID entity_id, const ComponentID component_id) const
{
   return m_storage.at(component_id).get_component_by_entity_id(entity_id);
}

bool EntityStorage::get_components(const EntityID entity_id, const std::span<const ComponentID> component_ids,
                                   std::span<void*> out_components) const
{
   if (component_ids.size() != out_components.size())
      return false;

   auto out_it = out_components.begin();
   for (const auto& cid : component_ids) {
      const auto& storage = m_storage.at(cid);
      *out_it = storage.get_component_by_entity_id(entity_id);
      if (*out_it == nullptr)
         return false;
      ++out_it;
   }

   return true;
}

void EntityStorage::allocate_component(const EntityID entity_id, const ComponentID component_id)
{
   m_storage.at(component_id).allocate_component(entity_id);
}

void EntityStorage::allocate_components(const EntityID entity_id, std::span<ComponentID> component_id)
{
   for (const auto cid : component_id) {
      m_storage.at(cid).allocate_component(entity_id);
   }
}

EntityID EntityStorage::allocate_entity(const EntityID parent)
{
   const auto child = m_top_entity_id++;
   m_hierarchy_tree.add_child(parent, child);
   return child;
}

bool EntityStorage::has_component(const EntityID entity_id, const ComponentID component_id) const
{
   return m_storage.at(component_id).contains_entity(entity_id);
}

ComponentStorage& EntityStorage::component_storage(const ComponentID component_id)
{
   assert(component_id < m_storage.size());
   return m_storage.at(component_id);
}

HierarchyTree& EntityStorage::hierarchy_tree()
{
   return m_hierarchy_tree;
}

const HierarchyTree& EntityStorage::hierarchy_tree() const
{
   return m_hierarchy_tree;
}

bool EntityStorage::serialize(io::IWriter& writer) const
{
   io::Serializer serializer(writer);
   if (!serializer.write_u32(static_cast<u32>(m_storage.size())))
      return false;

   for (const auto& storage : m_storage) {
      if (storage.is_empty())
         continue;

      if (!storage.serialize(writer))
         return false;
   }
   return true;
}

bool EntityStorage::deserialize(io::IReader& reader)
{
   assert(m_storage.empty() && "Can deserialize only on empty storage");

   io::Deserializer deserializer(reader);
   const u32 count = deserializer.read_u32();

   for (u32 i = 0; i < count; i++) {
      auto& storage = m_storage.emplace_back();
      if (!storage.deserialize(reader))
         return false;
   }

   return true;
}

}// namespace triglav::world
