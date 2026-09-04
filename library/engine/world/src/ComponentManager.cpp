#include "ComponentManager.hpp"

namespace triglav::world {

ComponentID ComponentManager::register_component(const ComponentInfo& info)
{
   const auto component_id = m_infos.size();
   m_name_to_id.emplace(info.component_class, component_id);
   m_infos.push_back(info);
   return component_id;
}

const ComponentInfo& ComponentManager::info_by_id(const ComponentID id) const
{
   assert(id <= m_infos.size());
   return m_infos.at(id);
}

const ComponentInfo* ComponentManager::info_by_class_name(const Name name) const
{
   const auto it = m_name_to_id.find(name);
   if (it == m_name_to_id.end()) {
      return nullptr;
   }
   return &this->info_by_id(it->second);
}

std::optional<ComponentID> ComponentManager::component_id_by_class_name(const Name name) const
{
   const auto it = m_name_to_id.find(name);
   if (it == m_name_to_id.end())
      return std::nullopt;
   return it->second;
}

bool ComponentManager::class_names_to_ids(const std::span<const Name> names, std::span<ComponentID> out_ids) const
{
   assert(names.size() == out_ids.size());

   auto out_it = out_ids.begin();
   for (const Name name : names) {
      const auto it = m_name_to_id.find(name);
      if (it == m_name_to_id.end())
         return false;
      *out_it = it->second;
      ++out_it;
   }

   return true;
}

mem_size ComponentManager::count() const
{
   return m_infos.size();
}

ComponentManager& ComponentManager::the()
{
   static ComponentManager instance;
   return instance;
}

ComponentRegisterer::ComponentRegisterer(const ComponentInfo& info)
{
   ComponentManager::the().register_component(info);
}

}// namespace triglav::world
