#include "LevelLoader.hpp"

#include "triglav/io/File.hpp"

#include <ryml.hpp>
#include <string>

namespace triglav::resource {

namespace {

Vector3 parse_vector3(const ryml::ConstNodeRef node)
{
   auto x = node["x"].val();
   auto y = node["y"].val();
   auto z = node["z"].val();

   return Vector3{
      std::stof(std::string{x.data(), x.size()}),
      std::stof(std::string{y.data(), y.size()}),
      std::stof(std::string{z.data(), z.size()}),
   };
}

Vector4 parse_vector4(const ryml::ConstNodeRef node)
{
   auto x = node["x"].val();
   auto y = node["y"].val();
   auto z = node["z"].val();
   auto w = node["w"].val();

   return Vector4{
      std::stof(std::string{x.data(), x.size()}),
      std::stof(std::string{y.data(), y.size()}),
      std::stof(std::string{z.data(), z.size()}),
      std::stof(std::string{w.data(), w.size()}),
   };
}

template<ResourceType CResType>
TypedName<CResType> to_typed_name(const ryml::csubstr str)
{
   if (std::ranges::all_of(str, [](const char c) { return std::isdigit(c); })) {
      return TypedName<CResType>{std::stoull(std::string{str.data(), str.size()})};
   }

   return TypedName<CResType>{make_rc_name(std::string_view{str.data(), str.size()})};
}

Transform3D parse_transformation(const ryml::ConstNodeRef node)
{
   const auto translation = node["translation"];
   const auto rotation = node["rotation"];
   const auto scale = node["scale"];

   const auto rotationVec4 = parse_vector4(rotation);

   return Transform3D{
      .rotation = {rotationVec4.w, rotationVec4.x, rotationVec4.y, rotationVec4.z},
      .scale = parse_vector3(scale),
      .translation = parse_vector3(translation),
   };
}

world::StaticMesh parse_static_mesh(const ryml::ConstNodeRef node)
{
   const auto meshName = node["mesh"].val();
   const auto name = node["name"].val();

   return world::StaticMesh{
      .meshName = to_typed_name<ResourceType::Mesh>(meshName),
      .name = {name.data(), name.size()},
      .transform = parse_transformation(node["transform"]),
   };
}

}// namespace

world::Level Loader<ResourceType::Level>::load(const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});

   world::Level result{};

   auto nodes = tree["nodes"];
   for (const auto node : nodes) {
      auto name = node["name"].val();

      world::LevelNode levelNode({name.data(), name.size()});

      auto items = node["items"];
      for (const auto item : items) {
         auto type = item["type"].val();
         if (type == "static_mesh") {
            levelNode.add_static_mesh(parse_static_mesh(item));
         }
      }

      result.add_node(make_name_id(std::string_view{name.data(), name.size()}), std::move(levelNode));
   }

   assert(result.save_to_file(io::Path{"/home/ego/debug.yaml"}));

   return result;
}

}// namespace triglav::resource
