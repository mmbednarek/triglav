#include "MaterialLoader.hpp"

#include "triglav/io/File.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::resource {

render_objects::Material Loader<ResourceType::Material>::load(ResourceManager& /*manager*/, const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});

   render_objects::Material material;
   material.deserialize_yaml(tree);

   return material;
}

}// namespace triglav::resource
