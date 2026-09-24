#include "AnimationLoader.hpp"

#include "triglav/asset/Asset.hpp"

namespace triglav::resource {

asset::Animation Loader<ResourceType::Animation>::load(const io::Path& path)
{
   const auto file = io::open_file(path, io::FileMode::Read);
   if (!file.has_value()) {
      assert(false);
      return {};
   }

   auto animation = asset::decode_animation(**file);
   assert(animation.has_value());

   // HACK TO SPEED UP THE ANIMATION
   for (auto& channel : animation->channels) {
      for (auto& timestamp : channel.timestamps) {
         timestamp *= 0.25f;
      }
   }

   return std::move(*animation);
}

void Loader<ResourceType::Animation>::collect_dependencies(std::set<ResourceName>& /*out_dependencies*/, const io::Path& /*path*/) {}

}// namespace triglav::resource
