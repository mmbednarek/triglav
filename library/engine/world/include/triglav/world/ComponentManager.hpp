#pragma once

#include "World.hpp"

#include "triglav/Name.hpp"
#include "triglav/String.hpp"

#include <map>
#include <vector>

namespace triglav::world {

struct ComponentInfo
{
   Name component_class;
   mem_size data_size;
   mem_size data_alignment;
   String name;
};

class ComponentManager
{
 public:
   ComponentID register_component(const ComponentInfo& info);
   [[nodiscard]] const ComponentInfo& info_by_id(ComponentID id) const;
   [[nodiscard]] const ComponentInfo* info_by_class_name(Name name) const;
   [[nodiscard]] std::optional<ComponentID> component_id_by_class_name(Name name) const;
   [[nodiscard]] bool class_names_to_ids(std::span<const Name> names, std::span<ComponentID> out_ids) const;
   [[nodiscard]] mem_size count() const;

   static ComponentManager& the();

 private:
   std::map<Name, ComponentID> m_name_to_id;
   std::vector<ComponentInfo> m_infos;
};

struct ComponentRegisterer
{
   explicit ComponentRegisterer(const ComponentInfo& info);
};

}// namespace triglav::world
