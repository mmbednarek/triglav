#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

#include "World.hpp"

namespace triglav::world {

struct StaticMesh
{
   std::string label;
   MeshName mesh_name;
   Transform3D transform;

   void create(Level& level, EntityID entity_id) const;
};

struct SkeletalMesh
{
   std::string label;
   MeshName mesh_name;
   Transform3D transform;
   ArmatureName armature_name;

   void create(Level& level, EntityID entity_id) const;
};

}// namespace triglav::world