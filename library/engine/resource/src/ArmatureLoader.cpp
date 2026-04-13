#include "ArmatureLoader.hpp"

#include "triglav/io/File.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/render_objects/Armature.hpp"

namespace triglav::resource {

render_objects::Armature Loader<ResourceType::Armature>::load(const io::Path& path)
{
   render_objects::BoneList bone_list;

   const auto file = io::open_file(path, io::FileMode::Read);
   assert(file.has_value());
   json_util::deserialize(bone_list.to_meta_ref(), **file);

   auto armature = render_objects::Armature{std::move(bone_list)};
   assert(armature.validate_armature());
   return armature;
}

void Loader<ResourceType::Armature>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/) {}

}// namespace triglav::resource
