#include "ResourceManager.h"

#include "Core.h"
#include "stb_image.h"

#include <algorithm>
#include <format>
#include <fstream>

#include "geometry/Mesh.h"

namespace {
std::vector<char> read_whole_file(const std::string_view name)
{
   std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
   if (not file.is_open()) {
      return {};
   }

   file.seekg(0, std::ios::end);
   const auto fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   std::vector<char> result{};
   result.resize(fileSize);

   file.read(result.data(), fileSize);
   return result;
}

}// namespace

namespace renderer {

ResourceManager::ResourceManager(graphics_api::Device &device, font::FontManger &fontManger) :
    m_device(device),
    m_fontManager(fontManger)
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

const Material &ResourceManager::material(Name assetName) const
{
   return m_materials.at(assetName);
}

const Model &ResourceManager::model(Name assetName) const
{
   return m_models.at(assetName);
}

const font::Typeface &ResourceManager::typeface(Name assetName) const
{
   return m_typefaces.at(assetName);
}

void ResourceManager::load_asset(const Name assetName, const std::string_view path)
{
   assert(not this->is_name_registered(assetName));

   switch (get_name_type(assetName)) {
   case NameType::Texture: m_textures.emplace(assetName, load_texture(path)); break;
   case NameType::Model: this->load_model(assetName, path); break;
   case NameType::FragmentShader:
      m_shaders.emplace(assetName, load_shader(graphics_api::ShaderStage::Fragment, path));
      break;
   case NameType::VertexShader:
      m_shaders.emplace(assetName, load_shader(graphics_api::ShaderStage::Vertex, path));
      break;
   case NameType::TypeFace: m_typefaces.emplace(assetName, load_typeface(path)); break;
   default: break;
   }
}

void ResourceManager::add_material(const Name assetName, const Material &material)
{
   assert(not this->is_name_registered(assetName));
   m_materials[assetName] = material;
}

void ResourceManager::add_mesh_and_model(Name assetName, geometry::DeviceMesh &model,
                                         const geometry::BoundingBox &boudingBox)
{
   Name meshName = (assetName & (~0b111ull)) | static_cast<uint64_t>(NameType::Mesh);
   m_meshes.emplace(meshName, std::move(model.mesh));

   std::vector<MaterialRange> ranges{};
   ranges.resize(model.ranges.size());
   std::transform(model.ranges.begin(), model.ranges.end(), ranges.begin(),
                  [](const geometry::MaterialRange &range) {
                     return MaterialRange{range.offset, range.size,
                                          make_name(std::format("mat:{}", range.materialName))};
                  });

   m_models.emplace(assetName, Model{meshName, boudingBox, std::move(ranges)});
}

void ResourceManager::add_mesh(const Name assetName, graphics_api::Mesh<geometry::Vertex> model)
{
   assert(not this->is_name_registered(assetName));
   m_meshes.emplace(assetName, std::move(model));
}

void ResourceManager::add_model(const Name assetName, Model model)
{
   assert(not this->is_name_registered(assetName));
   m_models.emplace(assetName, std::move(model));
}

void ResourceManager::add_texture(Name assetName, graphics_api::Texture texture)
{
   assert(not this->is_name_registered(assetName));
   m_textures.emplace(assetName, std::move(texture));
}

graphics_api::Texture ResourceManager::load_texture(const std::string_view path) const
{
   int texWidth, texHeight, texChannels;
   const stbi_uc *pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   assert(pixels != nullptr);

   auto texture = checkResult(
           m_device.create_texture(GAPI_FORMAT(RGBA, sRGB),
                                   {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)}));
   checkStatus(texture.write(m_device, pixels));
   return texture;
}

font::Typeface ResourceManager::load_typeface(const std::string_view path) const
{
   return m_fontManager.create_typeface(path, 0);
}

void ResourceManager::load_model(const Name model, const std::string_view path)
{
   const auto objMesh = geometry::Mesh::from_file(path);
   objMesh.triangulate();
   objMesh.recalculate_tangents();
   auto deviceMesh = objMesh.upload_to_device(m_device);

   Name meshName = (model & (~0b111ull)) | static_cast<uint64_t>(NameType::Mesh);
   m_meshes.emplace(meshName, std::move(deviceMesh.mesh));

   std::vector<MaterialRange> ranges{};
   ranges.resize(deviceMesh.ranges.size());
   std::transform(deviceMesh.ranges.begin(), deviceMesh.ranges.end(), ranges.begin(),
                  [](const geometry::MaterialRange &range) {
                     return MaterialRange{range.offset, range.size,
                                          make_name(std::format("mat:{}", range.materialName))};
                  });

   m_models.emplace(model, Model{meshName, objMesh.calculate_bouding_box(), std::move(ranges)});
}

graphics_api::Shader ResourceManager::load_shader(const graphics_api::ShaderStage stage,
                                                  const std::string_view path) const
{
   const auto data = read_whole_file(path);
   return checkResult(m_device.create_shader(stage, "main", data));
}

bool ResourceManager::is_name_registered(const Name assetName) const
{
   if (m_textures.contains(assetName))
      return true;
   if (m_meshes.contains(assetName))
      return true;
   if (m_shaders.contains(assetName))
      return true;
   if (m_materials.contains(assetName))
      return true;
   if (m_models.contains(assetName))
      return true;

   return false;
}

}// namespace renderer
