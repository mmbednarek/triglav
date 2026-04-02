#pragma once

#include "triglav/Math.hpp"
#include "triglav/meta/Meta.hpp"

#include <span>
#include <vector>

namespace triglav::render_objects {

using BoneID = u32;

constexpr BoneID BONE_ID_NO_PARENT = ~0u;

struct Bone
{
   TG_META_STRUCT_BODY(Bone)

   BoneID parent;
   Transform3D transform;
   Matrix4x4 inverse_bind;
};

struct BoneList
{
   TG_META_STRUCT_BODY(BoneList)

   std::vector<Bone> bones;
};

class Armature
{
 public:
   explicit Armature(BoneList&& bones);

   void add_bones(std::span<Bone> bones);
   [[nodiscard]] const Bone& bone(BoneID id) const;
   [[nodiscard]] Bone& bone(BoneID id);
   [[nodiscard]] MemorySize bone_count() const;

 private:
   std::vector<Bone> m_bones;
};

}// namespace triglav::render_objects
