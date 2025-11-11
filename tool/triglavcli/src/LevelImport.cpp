#include "LevelImport.hpp"

#include "MeshImport.hpp"
#include "ProjectConfig.hpp"
#include "ResourceList.hpp"
#include "TextureImport.hpp"

#include "triglav/gltf/Glb.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/render_objects/Material.hpp"
#include "triglav/world/Level.hpp"

#include <print>
#include <ryml.hpp>
#include <utility>

namespace c4 {
inline c4::substr to_substr(std::string& s) noexcept
{
   return {s.data(), s.size()};
}

inline c4::csubstr to_csubstr(std::string const& s) noexcept
{
   return {s.data(), s.size()};
}
}// namespace c4

namespace triglav::tool::cli {

using namespace name_literals;
using namespace io::path_literals;

namespace {

asset::FilterType to_filter_type(const gltf::SamplerFilter filter)
{
   switch (filter) {
   case gltf::SamplerFilter::Nearest:
      return asset::FilterType::NearestNeighbour;
   case gltf::SamplerFilter::Linear:
      return asset::FilterType::Linear;
   case gltf::SamplerFilter::NearestMipmapNearest:
      return asset::FilterType::NearestNeighbour;
   case gltf::SamplerFilter::LinearMipmapNearest:
      return asset::FilterType::Linear;
   case gltf::SamplerFilter::NearestMipmapLinear:
      return asset::FilterType::NearestNeighbour;
   case gltf::SamplerFilter::LinearMipmapLinear:
      return asset::FilterType::Linear;
   }
   assert(false);
   return {};
}

asset::TextureAddressMode to_address_mode(const gltf::SamplerWrap wrap)
{
   switch (wrap) {
   case gltf::SamplerWrap::ClampToEdge:
      return asset::TextureAddressMode::Clamp;
   case gltf::SamplerWrap::MirroredRepeat:
      return asset::TextureAddressMode::Mirror;
   case gltf::SamplerWrap::Repeat:
      return asset::TextureAddressMode::Repeat;
   }
   assert(false);
   return {};
}

asset::SamplerProperties to_sampler_properties(const gltf::Sampler& sampler)
{
   asset::SamplerProperties outSampler{};
   outSampler.minFilter = to_filter_type(sampler.minFilter);
   outSampler.magFilter = to_filter_type(sampler.magFilter);
   outSampler.addressModeU = to_address_mode(sampler.wrapS);
   outSampler.addressModeV = to_address_mode(sampler.wrapT);
   outSampler.addressModeW = asset::TextureAddressMode::Clamp;
   outSampler.enableAnisotropy = true;
   return outSampler;
}

std::string strip_extension(const std::string_view str)
{
   const auto dotAt = str.find_last_of('.');
   if (dotAt == std::string_view::npos) {
      return std::string{str};
   }
   return std::string{str.substr(0, dotAt)};
}

}// namespace

class LevelImporter
{
 public:
   LevelImporter(LevelImportProps props, ProjectInfo&& projectInfo, gltf::GlbResource&& glbResource, const io::Path& glbSrcPath) :
       m_props(std::move(props)),
       m_projectInfo(std::move(projectInfo)),
       m_glbFile(std::move(glbResource)),
       m_glbSourcePath(glbSrcPath)
   {
   }

   std::optional<TextureName> import_texture(const u32 textureID, const asset::TexturePurpose purpose)
   {
      if (m_importedTextures.contains(textureID)) {
         return m_importedTextures.at(textureID);
      }

      const auto& srcTexture = m_glbFile.document->textures.at(textureID);

      const auto& srcImage = m_glbFile.document->images.at(srcTexture.source);
      const auto& srcSampler = m_glbFile.document->samplers.at(srcTexture.sampler);

      auto textureNameStr =
         srcTexture.name.empty() ? std::format("{}.tex{}", strip_extension(m_props.srcPath.basename()), textureID) : srcTexture.name;
      for (char& ch : textureNameStr) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto importPath = m_projectInfo.default_import_path(ResourceType::Texture, textureNameStr);
      const auto rcName = make_rc_name(importPath);
      assert(rcName.type() == ResourceType::Texture);

      const auto dstPath = m_projectInfo.content_path(importPath);

      TextureImportProps importProps{
         .srcPath = "."_path,
         .dstPath = dstPath,
         .purpose = purpose,
         .samplerProperties = to_sampler_properties(srcSampler),
         .shouldCompress = true,
         .hasMipMaps = true,
         .shouldOverride = m_props.shouldOverride,
      };

      if (srcImage.uri.has_value()) {
         importProps.srcPath = m_glbSourcePath.parent().sub(srcImage.uri.value());
         if (!cli::import_texture(importProps)) {
            return std::nullopt;
         }
      } else if (srcImage.bufferView.has_value()) {
         auto stream = m_glbFile.bufferManager.buffer_view_to_stream(*srcImage.bufferView);
         if (!cli::import_texture_from_stream(importProps, stream)) {
            return std::nullopt;
         }
      } else {
         std::print(stderr, "triglav-cli: Failed to import texture {}, no URI or buffer view provided\n", textureNameStr);
         return std::nullopt;
      }

      if (!add_resource_to_index(importPath)) {
         return std::nullopt;
      }

      m_importedTextures.emplace(textureID, rcName);

      return rcName;
   }

   std::optional<MaterialName> import_material(const u32 materialID)
   {
      if (m_importedMaterials.contains(materialID)) {
         return m_importedMaterials.at(materialID);
      }

      const auto& srcMaterial = m_glbFile.document->materials.at(materialID);
      if (!srcMaterial.pbrMetallicRoughness.baseColorTexture.has_value()) {
         std::println(stderr, "triglav-cli: Failed to import GLTF material, material: {} has no texture assigned", materialID);
         return std::nullopt;
      }

      auto albedoTex = this->import_texture(srcMaterial.pbrMetallicRoughness.baseColorTexture->index, asset::TexturePurpose::Albedo);
      if (!albedoTex.has_value()) {
         return std::nullopt;
      }

      render_objects::Material dstMaterial{};
      if (srcMaterial.normalTexture.has_value()) {
         dstMaterial.materialTemplate = render_objects::MaterialTemplate::NormalMap;

         auto normalTex = this->import_texture(srcMaterial.normalTexture->index, asset::TexturePurpose::NormalMap);
         if (!normalTex.has_value()) {
            return std::nullopt;
         }

         render_objects::MTProperties_NormalMap properties{
            .albedo = *albedoTex,
            .normal = *normalTex,
            .roughness = srcMaterial.pbrMetallicRoughness.roughnessFactor,
            .metallic = srcMaterial.pbrMetallicRoughness.metallicFactor,
         };
         dstMaterial.properties = properties;
      } else {
         dstMaterial.materialTemplate = render_objects::MaterialTemplate::Basic;

         render_objects::MTProperties_Basic properties{
            .albedo = *albedoTex,
            .roughness = srcMaterial.pbrMetallicRoughness.roughnessFactor,
            .metallic = srcMaterial.pbrMetallicRoughness.metallicFactor,
         };
         dstMaterial.properties = properties;
      }

      auto materialNameStr =
         srcMaterial.name.empty() ? std::format("{}.mat{}", strip_extension(m_props.srcPath.basename()), materialID) : srcMaterial.name;
      for (char& ch : materialNameStr) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto importPath = m_projectInfo.default_import_path(ResourceType::Material, materialNameStr);
      const auto rcName = make_rc_name(importPath);
      assert(rcName.type() == ResourceType::Material);

      const auto dstPath = m_projectInfo.content_path(importPath);
      if (!m_props.shouldOverride && dstPath.exists()) {
         std::print(stderr, "triglav-cli: Failed to import material to {}, file exists\n", dstPath.string());
         return std::nullopt;
      }

      const auto file = io::open_file(dstPath, io::FileOpenMode::Create);
      if (!file.has_value()) {
         return std::nullopt;
      }

      ryml::Tree tree;
      ryml::NodeRef treeRef{tree};
      treeRef |= ryml::MAP;
      dstMaterial.serialize_yaml(treeRef);

      const auto str = ryml::emitrs_yaml<std::string>(tree);
      if (!(*file)->write({reinterpret_cast<const u8*>(str.data()), str.size()}).has_value()) {
         return std::nullopt;
      }

      std::print(stderr, "triglav-cli: Imported material to {}\n", importPath);

      if (!add_resource_to_index(importPath)) {
         return std::nullopt;
      }

      m_importedMaterials.emplace(materialID, rcName.name());

      return rcName;
   }

   [[nodiscard]] std::optional<MeshName> import_mesh(const std::optional<std::string>& name, const u32 meshID)
   {
      if (m_importedMeshes.contains(meshID)) {
         return m_importedMeshes.at(meshID);
      }

      auto meshNameStr = name.value_or(std::format("{}.mesh{}", strip_extension(m_props.srcPath.basename()), meshID));
      for (char& ch : meshNameStr) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto importPath = m_projectInfo.default_import_path(ResourceType::Mesh, meshNameStr);
      const auto rcName = make_rc_name(importPath);
      assert(rcName.type() == ResourceType::Mesh);

      const auto dstPath = m_projectInfo.content_path(importPath);
      if (!m_props.shouldOverride && dstPath.exists()) {
         std::print(stderr, "triglav-cli: Failed to import mesh to {}, file exists\n", dstPath.string());
         return std::nullopt;
      }

      const auto& mesh = m_glbFile.document->meshes.at(meshID);
      for (const auto& prim : mesh.primitives) {
         if (!prim.material.has_value())
            continue;
         if (!this->import_material(*prim.material).has_value())
            return std::nullopt;
      }

      auto gltfMesh = gltf::mesh_from_document(*m_glbFile.document, meshID, m_glbFile.bufferManager, m_importedMaterials);
      write_mesh_to_file(gltfMesh, dstPath);
      m_importedMeshes.emplace(meshID, rcName);

      std::print(stderr, "triglav-cli: Importing mesh to {}\n", importPath);

      if (!add_resource_to_index(importPath)) {
         return std::nullopt;
      }

      return rcName;
   }

   [[nodiscard]] bool import_scene()
   {
      if (!m_props.shouldOverride && m_props.dstPath.exists()) {
         std::print(stderr, "triglav-cli: Failed to import scene, file exists");
         return false;
      }

      const auto& glbScene = m_glbFile.document->scenes[m_glbFile.document->scene];

      world::LevelNode rootNode("root");
      for (const u32 nodeID : glbScene.nodes) {
         auto& glbNode = m_glbFile.document->nodes[nodeID];

         if (!glbNode.mesh.has_value())
            continue;

         auto transform = Transform3D::identity();
         if (glbNode.matrix.has_value()) {
            transform = Transform3D::from_matrix(glbNode.matrix.value());
         }
         if (glbNode.rotation.has_value()) {
            transform.rotation = {glbNode.rotation->w, glbNode.rotation->x, glbNode.rotation->y, glbNode.rotation->z};
         }
         if (glbNode.translation.has_value()) {
            transform.translation = glbNode.translation.value();
         }
         if (glbNode.scale.has_value()) {
            transform.scale = glbNode.scale.value();
         }

         auto meshName = this->import_mesh(glbNode.name, *glbNode.mesh);
         if (!meshName.has_value()) {
            return false;
         }

         world::StaticMesh mesh;
         mesh.name = glbNode.name.value_or(std::format("static_mesh{}", nodeID));
         mesh.transform = transform;
         mesh.meshName = *meshName;

         rootNode.add_static_mesh(std::move(mesh));
      }

      world::Level level;
      level.add_node("root"_name, std::move(rootNode));

      if (!level.save_to_file(m_props.dstPath)) {
         std::print(stderr, "triglav-cli: Failed to save level file\n");
         return false;
      }

      return true;
   }

 private:
   LevelImportProps m_props;
   ProjectInfo m_projectInfo;
   gltf::GlbResource m_glbFile;
   io::Path m_glbSourcePath;
   std::map<u32, MeshName> m_importedMeshes;
   std::map<u32, MaterialName> m_importedMaterials;
   std::map<u32, TextureName> m_importedTextures;
};

bool import_level(const LevelImportProps& props)
{
   std::print(stderr, "triglav-cli: Importing level to {}\n", props.dstPath.string());

   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      std::print(stderr, "triglav-cli: Failed to load project info\n");
      return false;
   }

   auto glbFile = gltf::open_glb_file(props.srcPath);
   if (!glbFile.has_value()) {
      std::print(stderr, "triglav-cli: Failed to load GLB file\n");
      return false;
   }

   LevelImporter importer(props, std::move(*projectInfo), std::move(*glbFile), props.srcPath);
   if (!importer.import_scene()) {
      return false;
   }

   return true;
}

}// namespace triglav::tool::cli
