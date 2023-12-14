#pragma once

#include <map>

#include "geometry/Mesh.h"
#include "graphics_api/Texture.h"

#include "Material.h"
#include "Model.h"
#include "Name.hpp"

namespace renderer {

class ResourceManager
{
 public:
   explicit ResourceManager(graphics_api::Device &device);

   void load_asset(Name assetName, std::string_view path);
   void add_material(Name assetName, Material material);
   void add_model(Name assetName, Model model);
   [[nodiscard]] const graphics_api::Texture &texture(Name assetName) const;
   [[nodiscard]] const graphics_api::Mesh<geometry::Vertex> &mesh(Name assetName) const;
   [[nodiscard]] const graphics_api::Shader &shader(Name assetName) const;
   [[nodiscard]] const Material &material(Name assetName) const;
   [[nodiscard]] const Model &model(Name assetName) const;

   [[nodiscard]] bool is_name_registered(Name assetName) const;

 private:
   [[nodiscard]] graphics_api::Texture load_texture(std::string_view path) const;
   [[nodiscard]] graphics_api::Shader load_shader(graphics_api::ShaderStage stage, std::string_view path) const;
   void load_model(Name model, std::string_view path);

   std::map<Name, graphics_api::Texture> m_textures;
   std::map<Name, graphics_api::Mesh<geometry::Vertex>> m_meshes;
   std::map<Name, graphics_api::Shader> m_shaders;
   std::map<Name, Material> m_materials;
   std::map<Name, Model> m_models;
   graphics_api::Device &m_device;
};

}// namespace renderer