#include "Level.hpp"

#include "World.hpp"

#include "triglav/Ranges.hpp"
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

void Level::add_node(const Name id, LevelNode&& node)
{
   std::array comp_names{
      "triglav::Transform3D"_name,        "triglav::world::Mesh"_name,     "triglav::world::Tag"_name,
      "triglav::world::EntityLabel"_name, "triglav::world::Armature"_name,
   };
   std::array<void*, 5> component_ptrs{};
   std::array<ComponentID, 5> component_ids{};

   ensure_component_registration();

   assert(ComponentManager::the().class_names_to_ids(comp_names, component_ids));

   for (const auto& static_mesh : node.static_meshes()) {
      std::span<ComponentID> comp_id_span = component_ids;
      std::span<void*> ptr_span = component_ptrs;
      if (!static_mesh.armature_name.has_value()) {
         comp_id_span = comp_id_span.subspan(0, 4);
         ptr_span = ptr_span.subspan(0, 4);
      }

      const EntityID entity_id = m_entity_storage.allocate_entity(0);
      m_entity_storage.allocate_components(entity_id, comp_id_span);

      assert(m_entity_storage.get_components(entity_id, comp_id_span, ptr_span));

      *static_cast<Transform3D*>(component_ptrs[0]) = static_mesh.transform;
      static_cast<Mesh*>(component_ptrs[1])->name = static_mesh.mesh_name;
      static_cast<Tag*>(component_ptrs[2])->tag = id;
      static_cast<EntityLabel*>(component_ptrs[3])->label = static_mesh.name;
      if (static_mesh.armature_name.has_value()) {
         static_cast<Armature*>(component_ptrs[4])->name = *static_mesh.armature_name;
      }

      for (const auto component_id : comp_id_span) {
         m_addition_lists[component_id].emplace_back(entity_id);
      }
   }

   m_nodes.emplace(id, std::move(node));
}

void Level::init_entities()
{
   m_entity_storage.init();
}

LevelNode& Level::at(const Name id)
{
   return m_nodes.at(id);
}

LevelNode& Level::root()
{
   using namespace name_literals;
   return this->at("root"_name);
}

meta::Ref Level::component_ref(const EntityID entity_id, const Name component_name) const
{
   const auto comp_id = ComponentManager::the().component_id_by_class_name(component_name);
   if (!comp_id.has_value())
      return {nullptr, component_name};

   void* ptr = m_entity_storage.get_component(entity_id, *comp_id);
   return {ptr, component_name};
}

EntityID Level::new_entity(const EntityID parent)
{
   return m_entity_storage.allocate_entity(parent);
}

HierarchyTree::Range Level::children_of(const EntityID entity) const
{
   return m_entity_storage.hierarchy_tree().children_of(entity);
}

EntityID Level::parent_of(const EntityID entity) const
{
   return m_entity_storage.hierarchy_tree().parent_of(entity);
}

void Level::register_system(ISystem& system, std::span<Name> component_class_names)
{
   std::set<ComponentID> component_ids;
   for (const auto name : component_class_names) {
      const auto comp_id = ComponentManager::the().component_id_by_class_name(name);
      if (!comp_id.has_value())
         continue;
      component_ids.insert(*comp_id);
   }

   m_systems.push_back(SystemRegistration{
      .system = &system,
      .component_ids = std::move(component_ids),
   });
}

void Level::flush()
{
   for (const auto& [component_id, entities] : m_addition_lists) {
      for (const auto& system : m_systems) {
         if (!system.component_ids.contains(component_id))
            continue;

         system.system->on_added_component(ComponentManager::the().info_by_id(component_id).component_class, component_id, entities);
      }
   }
   m_addition_lists.clear();

   for (const auto& [component_id, entities] : m_change_lists) {
      for (const auto& system : m_systems) {
         if (!system.component_ids.contains(component_id))
            continue;

         system.system->on_modified_component(ComponentManager::the().info_by_id(component_id).component_class, component_id, entities);
      }
   }
   m_change_lists.clear();
}

void Level::insert_component_change(const ComponentID id, const EntityID entity)
{
   m_change_lists[id].emplace_back(entity);
}

void Level::serialize_yaml(c4::yml::NodeRef& node) const
{
   auto nodes_yaml = node["nodes"];
   nodes_yaml |= ryml::SEQ;
   for (const auto& level_node : Values(m_nodes)) {
      auto child = nodes_yaml.append_child();
      child |= ryml::MAP;
      level_node.serialize_yaml(child);
   }
}

bool Level::save_to_file(const io::Path& path) const
{
   const auto file = io::open_file(path, io::FileMode::Write | io::FileMode::Create);
   if (!file.has_value()) {
      return false;
   }

   ryml::Tree tree;
   ryml::NodeRef tree_ref{tree};
   tree_ref |= ryml::MAP;
   this->serialize_yaml(tree_ref);

   const auto str = ryml::emitrs_yaml<std::string>(tree);
   return (*file)->write({reinterpret_cast<const u8*>(str.data()), str.size()}).has_value();
}

}// namespace triglav::world