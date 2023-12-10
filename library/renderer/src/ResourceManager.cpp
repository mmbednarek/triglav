#include "ResourceManager.h"

#include "Core.h"
#include "stb_image.h"

#include <fstream>

namespace {
std::vector<uint8_t> read_whole_file(const std::string_view name)
{
   std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
   if (not file.is_open()) {
      return {};
   }

   file.seekg(0, std::ios::end);
   const auto fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   std::vector<uint8_t> result{};
   result.resize(fileSize);

   file.read(reinterpret_cast<char *>(result.data()), fileSize);
   return result;
}

}// namespace

namespace renderer {

ResourceManager::ResourceManager(graphics_api::Device &device) :
    m_device(device)
{
}

const graphics_api::Texture &ResourceManager::texture(const Name assetName) const
{
   return m_textures.at(assetName);
}

const graphics_api::Mesh<geometry::Vertex> &ResourceManager::mesh(const Name assetName) const
{
   return m_meshes.at(assetName);
}

const graphics_api::Shader &ResourceManager::shader(const Name assetName) const
{
   return m_shaders.at(assetName);
}

void ResourceManager::load_asset(const Name assetName, const std::string_view path)
{
   switch (get_name_type(assetName)) {
   case NameType::Texture: m_textures.emplace(assetName, load_texture(path)); break;
   case NameType::Mesh: m_meshes.emplace(assetName, load_mesh(path)); break;
   case NameType::FragmentShader:
      m_shaders.emplace(assetName, load_shader(graphics_api::ShaderStage::Fragment, path));
      break;
   case NameType::VertexShader:
      m_shaders.emplace(assetName, load_shader(graphics_api::ShaderStage::Vertex, path));
      break;
   }
}

graphics_api::Texture ResourceManager::load_texture(const std::string_view path) const
{
   int texWidth, texHeight, texChannels;
   const stbi_uc *pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   assert(pixels != nullptr);

   auto texture = checkResult(
           m_device.create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB),
                                   {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)}));
   checkStatus(texture.write(m_device, pixels));
   return texture;
}

graphics_api::Mesh<geometry::Vertex> ResourceManager::load_mesh(const std::string_view path) const
{
   const auto objMesh = geometry::Mesh::from_file(path);
   objMesh.triangulate();
   return objMesh.upload_to_device(m_device);
}

graphics_api::Shader ResourceManager::load_shader(const graphics_api::ShaderStage stage,
                                                  const std::string_view path) const
{
   const auto data = read_whole_file(path);
   return checkResult(m_device.create_shader(stage, "main", data));
}

}// namespace renderer
