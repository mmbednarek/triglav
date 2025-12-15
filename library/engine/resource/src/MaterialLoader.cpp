#include "MaterialLoader.hpp"

#include "ResourceManager.hpp"

#include "triglav/io/File.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::resource {

namespace {

render_objects::Material load_material(const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});

   render_objects::Material material;
   material.deserialize_yaml(tree);

   return material;
}

}// namespace

render_objects::Material Loader<ResourceType::Material>::load(ResourceManager& /*manager*/, const io::Path& path)
{
   return load_material(path);
}

void Loader<ResourceType::Material>::collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path)
{
   const auto mat = load_material(path);
   if (std::holds_alternative<render_objects::MTProperties_Basic>(mat.properties)) {
      const auto& props = std::get<render_objects::MTProperties_Basic>(mat.properties);
      out_dependencies.insert(props.albedo);
   } else if (std::holds_alternative<render_objects::MTProperties_NormalMap>(mat.properties)) {
      const auto& props = std::get<render_objects::MTProperties_NormalMap>(mat.properties);
      out_dependencies.insert(props.albedo);
      out_dependencies.insert(props.normal);
   } else if (std::holds_alternative<render_objects::MTProperties_FullPBR>(mat.properties)) {
      const auto& props = std::get<render_objects::MTProperties_FullPBR>(mat.properties);
      out_dependencies.insert(props.texture);
      out_dependencies.insert(props.normal);
      out_dependencies.insert(props.roughness);
      out_dependencies.insert(props.metallic);
   }
}

}// namespace triglav::resource
