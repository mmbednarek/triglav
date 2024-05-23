#include "MaterialLoader.h"

#include "triglav/io/File.h"
#include "triglav/render_core/Material.hpp"

#include <ranges>
#include <ryml.hpp>
#include <string>

namespace triglav::resource {

render_core::MaterialTemplate Loader<ResourceType::MaterialTemplate>::load(const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());


   auto tree = ryml::parse_in_place(c4::substr{const_cast<char *>(path.string().data()), path.string().size()},
                                    c4::substr{file.data(), file.size()});

   auto fragmentShader = tree["fragment_shader"].val();
   auto vertexShader   = tree["vertex_shader"].val();

   render_core::MaterialTemplate result{
           .fragmentShader{make_rc_name({fragmentShader.data(), fragmentShader.size()})},
           .vertexShader{make_rc_name({vertexShader.data(), vertexShader.size()})}};

   auto properties = tree["properties"];

   for (const auto &property : properties) {
      auto nameStr = property["name"].val();
      auto typeStr = property["type"].val();

      render_core::MaterialPropertyType type{};
      if (typeStr == "texture2D") {
         type = render_core::MaterialPropertyType::Texture2D;
      } else if (typeStr == "float32") {
         type = render_core::MaterialPropertyType::Float32;
      } else if (typeStr == "vec3") {
         type = render_core::MaterialPropertyType::Vec3;
      } else if (typeStr == "vec4") {
         type = render_core::MaterialPropertyType::Vec4;
      }

      result.properties.emplace_back(make_name_id(std::string_view{nameStr.data(), nameStr.size()}), type);
   }

   return result;
}

render_core::Material Loader<ResourceType::Material>::load(ResourceManager &manager, const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree = ryml::parse_in_place(c4::substr{const_cast<char *>(path.string().data()), path.string().size()},
                                    c4::substr{file.data(), file.size()});

   auto templateStr = tree["template"].val();
   auto templateName = make_rc_name({templateStr.data(), templateStr.size()});

   auto &materialTemplate = manager.get<ResourceType::MaterialTemplate>(templateName);

   std::vector<render_core::MaterialPropertyValue> values;
   values.resize(materialTemplate.properties.size(), 0.0f);

   auto properties = tree["properties"];
   for (const auto &property : properties) {
      auto nameStr = property.key();

      auto name    = make_name_id({nameStr.data(), nameStr.size()});

      auto it = std::ranges::find_if(materialTemplate.properties, [target = name](const render_core::MaterialProperty &prop) { return prop.name == target; });
      if (it == materialTemplate.properties.end())
         continue;

      auto index = it - materialTemplate.properties.begin();

      switch (it->type) {
      case render_core::MaterialPropertyType::Texture2D: {
         auto valueStr = property.val();
         values[index] = TextureName{make_rc_name({valueStr.data(), valueStr.size()})};
         break;
      }
      case render_core::MaterialPropertyType::Float32: {
         auto valueStr = property.val();
         values[index] = std::stof({valueStr.data(), valueStr.size()});
         break;
      }
      case render_core::MaterialPropertyType::Vec3: {
         const auto xStr = property["x"].val();
         const auto yStr = property["y"].val();
         const auto zStr = property["z"].val();
         values[index] = glm::vec3{
                 std::stof({xStr.data(), xStr.size()}),
                 std::stof({yStr.data(), yStr.size()}),
                 std::stof({zStr.data(), zStr.size()}),
         };
         break;
      }
      case render_core::MaterialPropertyType::Vec4: {
         const auto xStr = property["x"].val();
         const auto yStr = property["y"].val();
         const auto zStr = property["z"].val();
         const auto wStr = property["w"].val();
         values[index] = glm::vec4{
                 std::stof({xStr.data(), xStr.size()}),
                 std::stof({yStr.data(), yStr.size()}),
                 std::stof({zStr.data(), zStr.size()}),
                 std::stof({wStr.data(), wStr.size()}),
         };
         break;
      }
      }
   }

   return render_core::Material{.materialTemplate = templateName, .values = std::move(values)};
}

}// namespace triglav::resource
