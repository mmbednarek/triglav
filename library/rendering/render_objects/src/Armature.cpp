#include "Armature.hpp"

namespace triglav::render_objects {

Armature::Armature(BoneList&& bones) :
    m_bones(std::move(bones.bones))
{
}

void Armature::add_bones(std::span<Bone> bones)
{
   m_bones.append_range(bones);
}

const Bone& Armature::bone(const BoneID id) const
{
   return m_bones.at(id);
}

Bone& Armature::bone(const BoneID id)
{
   return m_bones.at(id);
}

MemorySize Armature::bone_count() const
{
   return m_bones.size();
}

}// namespace triglav::render_objects

#define TG_TYPE(NS) NS(NS(triglav, render_objects), Bone)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(parent, triglav::u32)
TG_META_PROPERTY(transform, triglav::Transform3D)
TG_META_PROPERTY(inverse_bind, triglav::Matrix4x4)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, render_objects), BoneList)
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(bones, triglav::render_objects::Bone)
TG_META_CLASS_END
#undef TG_TYPE
