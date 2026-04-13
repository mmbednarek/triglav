#include "Armature.hpp"

#include "triglav/Logging.hpp"

#include <queue>

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

void Armature::recalculate_inverse_bind_matrices()
{
   std::vector<Matrix4x4> calculated_mats(m_bones.size());
   std::vector<bool> visited(m_bones.size());

   std::queue<BoneID> deferred;
   BoneID id = 0;
   for (Bone& bone : m_bones) {
      if (bone.parent == BONE_ID_NO_PARENT) {
         calculated_mats[id] = bone.transform.to_matrix();
         visited[id] = true;
         ++id;
         continue;
      }

      if (!visited[bone.parent]) {
         deferred.push(id);
         ++id;
         continue;
      }

      calculated_mats[id] = calculated_mats[bone.parent] * bone.transform.to_matrix();
      visited[id] = true;
      ++id;
   }

   while (!deferred.empty()) {
      const auto bone_id = deferred.front();
      const auto& bone = m_bones[bone_id];
      deferred.pop();

      if (!visited[bone.parent]) {
         deferred.push(bone_id);
         continue;
      }

      calculated_mats[bone_id] = calculated_mats[bone.parent] * bone.transform.to_matrix();
   }

   for (id = 0; id < m_bones.size(); ++id) {
      m_bones[id].inverse_bind = glm::inverse(calculated_mats[id]);
   }
}

namespace {

float mat4_error(const Matrix4x4& left, const Matrix4x4& right)
{
   float error = 0.0f;
   for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
         error += std::abs(left[y][x] - right[y][x]);
      }
   }
   return error / 16.0f;
}

}// namespace

bool Armature::validate_armature() const
{
   static constexpr auto TOLERANCE = 0.01f;

   for (BoneID id = 0; id < m_bones.size(); ++id) {
      auto mat = Matrix4x4{1};
      auto parent = id;
      while (parent != BONE_ID_NO_PARENT) {
         mat = m_bones[parent].transform.to_matrix() * mat;
         parent = m_bones[parent].parent;
      }

      const auto test = mat * m_bones[id].inverse_bind;
      const auto err = mat4_error(test, Matrix4x4{1});
      if (err > TOLERANCE) {
         return false;
      }
   }
   return true;
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
