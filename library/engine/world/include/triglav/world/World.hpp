#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::world {

using EntityID = u32;
using ComponentID = u32;

constexpr EntityID ROOT_ENTITY = 0u;
constexpr EntityID NO_ENTITY = ~0u;

struct Mesh
{
   TG_META_STRUCT_BODY(Mesh)
   MeshName name;
};

struct Armature
{
   TG_META_STRUCT_BODY(Armature)
   ArmatureName name;
};

struct Tag
{
   TG_META_STRUCT_BODY(Tag)
   Name tag;
};

struct EntityLabel
{
   TG_META_STRUCT_BODY(EntityLabel)
   std::string label;
};

class Level;

class ISystem
{
 public:
   virtual ~ISystem() = default;
   virtual void on_removed_entities(std::span<const EntityID> ids) = 0;
   virtual void on_added_component(Name component_name, ComponentID component_id, std::span<const EntityID> entities) = 0;
   virtual void on_modified_component(Name component_name, ComponentID component_id, std::span<const EntityID> entities) = 0;
};

[[maybe_unused]] void ensure_component_registration();

}// namespace triglav::world