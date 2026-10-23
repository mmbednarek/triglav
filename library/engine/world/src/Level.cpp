#include "Level.hpp"

#include "World.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"

#include <c4/substr.hpp>
#include <ryml.hpp>

namespace c4 {
inline c4::substr to_substr(std::string& s) noexcept
{
   return {s.data(), s.size()};
}

inline c4::csubstr to_csubstr(std::string const& s) noexcept
{
   return {s.data(), s.size()};
}
}// namespace c4

namespace triglav::world {

using namespace name_literals;

void Level::init_entities()
{
   m_entity_storage.init();
}

void Level::on_loaded_resources()
{
   for (const auto& [sys_name, system] : m_systems) {
      system.system->on_level_loaded(*this);
   }
}

meta::Ref Level::component_ref(const EntityID entity_id, const Name component_name) const
{
   const auto comp_id = ComponentManager::the().component_id_by_class_name(component_name);
   if (!comp_id.has_value())
      return {nullptr, component_name};

   void* ptr = m_entity_storage.get_component(entity_id, *comp_id);
   return {ptr, component_name};
}

bool Level::deserialize(io::IReader& reader)
{
   const auto header = asset::decode_header(reader);
   if (!header.has_value())
      return false;

   return m_entity_storage.deserialize(reader);
}

EntityID Level::new_entity(const EntityID parent)
{
   return m_entity_storage.allocate_entity(parent);
}

void* Level::component_raw(const EntityID entity_id, const ComponentID component_id) const
{
   return m_entity_storage.get_component(entity_id, component_id);
}

void* Level::component_mut_raw(const EntityID entity_id, const ComponentID component_id)
{
   this->insert_component_change(component_id, entity_id);
   return m_entity_storage.get_component(entity_id, component_id);
}

void Level::remove_entity(const EntityID entity)
{
   m_removal_list.emplace_back(entity);
}

HierarchyTree::Range Level::children_of(const EntityID entity) const
{
   return m_entity_storage.hierarchy_tree().children_of(entity);
}

EntityID Level::parent_of(const EntityID entity) const
{
   return m_entity_storage.hierarchy_tree().parent_of(entity);
}

void Level::register_system(std::unique_ptr<ISystem> system, const std::span<const Name> component_class_names)
{
   std::set<ComponentID> component_ids;
   for (const auto name : component_class_names) {
      const auto comp_id = ComponentManager::the().component_id_by_class_name(name);
      if (!comp_id.has_value())
         continue;
      component_ids.insert(*comp_id);
   }

   const auto tag = system->system_name();
   m_systems.emplace(tag, SystemRegistration{
                             .system = std::move(system),
                             .component_ids = std::move(component_ids),
                          });
}

void Level::flush()
{
   for (const auto& [component_id, entities] : m_addition_lists) {
      for (const auto& [sys_name, system] : m_systems) {
         if (!system.component_ids.contains(component_id))
            continue;

         system.system->on_added_component(*this, ComponentManager::the().info_by_id(component_id).component_class, component_id, entities);
      }
   }
   m_addition_lists.clear();

   for (const auto& [component_id, entities] : m_change_lists) {
      for (const auto& [sys_name, system] : m_systems) {
         if (!system.component_ids.contains(component_id))
            continue;

         system.system->on_modified_component(*this, ComponentManager::the().info_by_id(component_id).component_class, component_id,
                                              entities);
      }
   }
   m_change_lists.clear();
}

void Level::insert_component_change(const ComponentID id, const EntityID entity)
{
   auto& change_list = m_change_lists[id];
   auto it = std::lower_bound(change_list.begin(), change_list.end(), entity);
   if (it == change_list.end() || *it != entity) {
      change_list.insert(it, entity);
   }
}

bool Level::save_to_file(const io::Path& path) const
{
   const auto file = io::open_file(path, io::FileMode::Write | io::FileMode::Create);
   if (!file.has_value()) {
      return false;
   }

   asset::write_header(**file, ResourceType::Level);

   return m_entity_storage.serialize(**file);
}

}// namespace triglav::world