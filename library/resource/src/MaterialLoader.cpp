#include "MaterialLoader.h"

#include "triglav/io/File.h"
#include "triglav/render_core/Material.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::resource {

render_core::Material Loader<ResourceType::Material>::load(std::string_view path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree = ryml::parse_in_place(c4::substr{const_cast<char *>(path.data()), path.size()},
                                    c4::substr{file.data(), file.size()});

   auto properties    = tree["properties"];
   auto albedoTexture = properties["albedo_texture"].val();
   bool hasNormal{false};
   c4::csubstr normalTexture{};
   if (properties.has_child("normal_texture")) {
      hasNormal     = true;
      normalTexture = properties["normal_texture"].val();
   } else {
      normalTexture = "quartz/normal.tex";
   }
   auto roughnessStr = properties["roughness"].val();
   auto roughness    = std::stof(std::string{roughnessStr.data(), roughnessStr.size()});
   auto metallicStr  = properties["metallic"].val();
   auto metallic     = std::stof(std::string{metallicStr.data(), metallicStr.size()});

   // clang-format off
   return render_core::Material{
           .texture = make_name(std::string_view{albedoTexture.data(),albedoTexture.size()}),
           .normal_texture = make_name(std::string_view{normalTexture.data(),normalTexture.size()}),
           .props = render_core::MaterialProps{
              hasNormal,
              roughness,
          metallic,
           },
   };
   // clang-format on
}

}// namespace triglav::resource
