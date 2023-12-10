#pragma once

#include <map>

#include "geometry/Mesh.h"
#include "graphics_api/Texture.h"
#include "Name.hpp"

namespace renderer {

/*
 -- RESOURCE LIST --

 tex:house "texture/house.png"
*/

class ResourceManager
{
 public:
   explicit ResourceManager(graphics_api::Device &device);

   void load_asset(Name assetName, std::string_view path);
   [[nodiscard]] const graphics_api::Texture &texture(Name assetName) const;
   [[nodiscard]] const graphics_api::Mesh<geometry::Vertex> &mesh(Name assetName) const;
   [[nodiscard]] const graphics_api::Shader &shader(Name assetName) const;

 private:
   [[nodiscard]] graphics_api::Texture load_texture(std::string_view path) const;
   [[nodiscard]] graphics_api::Mesh<geometry::Vertex> load_mesh(std::string_view path) const;
   [[nodiscard]] graphics_api::Shader load_shader(graphics_api::ShaderStage stage, std::string_view path) const;

   std::map<Name, graphics_api::Texture> m_textures;
   std::map<Name, graphics_api::Mesh<geometry::Vertex>> m_meshes;
   std::map<Name, graphics_api::Shader> m_shaders;
   graphics_api::Device &m_device;
};

}// namespace renderer