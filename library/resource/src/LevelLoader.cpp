#include "LevelLoader.h"

#include "triglav/io/File.h"

#include <ryml.hpp>

namespace triglav::resource {

namespace {

glm::vec3 parse_vector3(const ryml::ConstNodeRef node)
{
   auto x = node["x"].val();
   auto y = node["y"].val();
   auto z = node["z"].val();

   return glm::vec3{
           std::stof(x.data()),
           std::stof(y.data()),
           std::stof(z.data()),
   };
}

glm::vec3 parse_angle3(const ryml::ConstNodeRef node)
{
   auto x = node["roll"].val();
   auto y = node["pitch"].val();
   auto z = node["yaw"].val();

   return glm::vec3{
           std::stof(x.data()),
           std::stof(y.data()),
           std::stof(z.data()),
   };
}

world::Transformation parse_transformation(const ryml::ConstNodeRef node)
{
   const auto position = node["position"];
   const auto rotation = node["rotation"];
   const auto scale    = node["scale"];

   return world::Transformation{
           .position = parse_vector3(position),
           .rotation = parse_angle3(rotation),
           .scale    = parse_vector3(scale),
   };
}

world::StaticMesh parse_static_mesh(const ryml::ConstNodeRef node)
{
   auto meshName = node["mesh"].val();
   return world::StaticMesh{
           .meshName  = make_name(std::string_view{meshName.data(), meshName.size()}),
           .transform = parse_transformation(node["transform"]),
   };
}

}// namespace

world::Level Loader<ResourceType::Level>::load(std::string_view path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree = ryml::parse_in_place(c4::substr{const_cast<char *>(path.data()), path.size()},
                                    c4::substr{file.data(), file.size()});

   world::Level result{};

   auto nodes = tree["nodes"];
   for (const auto node : nodes) {
      auto name = node["name"].val();

      world::LevelNode levelNode;

      auto items = node["items"];
      for (const auto item : items) {
         auto type = item["type"].val();
         if (type == "static_mesh") {
            levelNode.add_static_mesh(parse_static_mesh(item));
         }
      }

      result.add_node(make_name_id(std::string_view{name.data(), name.size()}), std::move(levelNode));
   }

   return result;
}

}// namespace triglav::resource
