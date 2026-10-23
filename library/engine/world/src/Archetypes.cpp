#include "Archetypes.hpp"

#include "Level.hpp"

namespace triglav::world {

void StaticMesh::create(Level& level, const EntityID entity_id) const
{
   level.insert_component<EntityLabel>(entity_id)->label = this->label;
   level.insert_component<Tag>(entity_id)->tag = make_name_id(this->label);
   level.insert_component<Mesh>(entity_id)->name = this->mesh_name;
   *level.insert_component<Transform3D>(entity_id) = this->transform;
}

void SkeletalMesh::create(Level& level, const EntityID entity_id) const
{
   level.insert_component<EntityLabel>(entity_id)->label = this->label;
   level.insert_component<Tag>(entity_id)->tag = make_name_id(this->label);
   level.insert_component<Mesh>(entity_id)->name = this->mesh_name;
   *level.insert_component<Transform3D>(entity_id) = this->transform;
   level.insert_component<Armature>(entity_id)->name = this->armature_name;
}

}// namespace triglav::world
