#include "LevelImport.hpp"

#include "MeshImport.hpp"
#include "ResourceList.hpp"
#include "TextureImport.hpp"

#include "triglav/gltf/Glb.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/project/Project.hpp"
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
   asset::SamplerProperties out_sampler{};
   out_sampler.min_filter = to_filter_type(sampler.min_filter);
   out_sampler.mag_filter = to_filter_type(sampler.mag_filter);
   out_sampler.address_mode_u = to_address_mode(sampler.wrap_s);
   out_sampler.address_mode_v = to_address_mode(sampler.wrap_t);
   out_sampler.address_mode_w = asset::TextureAddressMode::Clamp;
   out_sampler.enable_anisotropy = true;
   return out_sampler;
}

std::string strip_extension(const std::string_view str)
{
   const auto dot_at = str.find_last_of('.');
   if (dot_at == std::string_view::npos) {
      return std::string{str};
   }
   return std::string{str.substr(0, dot_at)};
}

}// namespace

class LevelImporter
{
 public:
   LevelImporter(LevelImportProps props, project::ProjectInfo&& project_info, gltf::GlbResource&& glb_resource,
                 const io::Path& glb_src_path) :
       m_props(std::move(props)),
       m_project_info(std::move(project_info)),
       m_glb_file(std::move(glb_resource)),
       m_glb_source_path(glb_src_path)
   {
   }

   std::optional<TextureName> import_texture(const u32 texture_id, const asset::TexturePurpose purpose,
                                             const std::optional<TextureChannel> extract_channel = {})
   {
      const auto tex_iden = std::make_pair(texture_id, extract_channel);
      if (m_imported_textures.contains(tex_iden)) {
         return m_imported_textures.at(tex_iden);
      }

      const auto& src_texture = m_glb_file.document->textures.at(texture_id);

      const auto& src_image = m_glb_file.document->images.at(src_texture.source);
      const auto& src_sampler = m_glb_file.document->samplers.at(src_texture.sampler);

      auto texture_name_str =
         src_texture.name.empty() ? std::format("{}.tex{}", strip_extension(m_props.src_path.basename()), texture_id) : src_texture.name;
      for (char& ch : texture_name_str) {
         ch = static_cast<char>(std::tolower(ch));
      }
      if (extract_channel.has_value()) {
         switch (*extract_channel) {
         case TextureChannel::Red:
            texture_name_str.append(".red");
            break;
         case TextureChannel::Green:
            texture_name_str.append(".green");
            break;
         case TextureChannel::Blue:
            texture_name_str.append(".blue");
            break;
         case TextureChannel::Alpha:
            texture_name_str.append(".alpha");
            break;
         }
      }

      auto import_path = m_project_info.default_import_path(ResourceType::Texture, texture_name_str);
      const auto rc_name = make_rc_name(import_path);
      assert(rc_name.type() == ResourceType::Texture);

      const auto dst_path = m_project_info.content_path(import_path);

      TextureImportProps import_props{
         .src_path = "."_path,
         .dst_path = dst_path,
         .purpose = purpose,
         .sampler_properties = to_sampler_properties(src_sampler),
         .should_compress = true,
         .has_mip_maps = true,
         .should_override = m_props.should_override,
         .extract_channel = extract_channel,
      };

      if (src_image.uri.has_value()) {
         import_props.src_path = m_glb_source_path.parent().sub(src_image.uri.value());
         if (!cli::import_texture(import_props)) {
            return std::nullopt;
         }
      } else if (src_image.buffer_view.has_value()) {
         auto stream = m_glb_file.buffer_manager.buffer_view_to_stream(*src_image.buffer_view);
         if (!cli::import_texture_from_stream(import_props, stream)) {
            return std::nullopt;
         }
      } else {
         std::print(stderr, "triglav-cli: Failed to import texture {}, no URI or buffer view provided\n", texture_name_str);
         return std::nullopt;
      }

      if (!add_resource_to_index(import_path)) {
         return std::nullopt;
      }

      m_imported_textures.emplace(tex_iden, rc_name);

      return rc_name;
   }

   std::optional<MaterialName> import_material(const u32 material_id)
   {
      if (m_imported_materials.contains(material_id)) {
         return m_imported_materials.at(material_id);
      }

      const auto& src_material = m_glb_file.document->materials.at(material_id);
      if (!src_material.pbr_metallic_roughness.base_color_texture.has_value()) {
         std::println(stderr, "triglav-cli: Failed to import GLTF material, material: {} has no texture assigned, defaulting to stone.mat",
                      material_id);
         return {"material/stone.mat"_rc};
      }

      auto albedo_tex = this->import_texture(src_material.pbr_metallic_roughness.base_color_texture->index, asset::TexturePurpose::Albedo);
      if (!albedo_tex.has_value()) {
         return std::nullopt;
      }

      render_objects::Material dst_material{};
      if (src_material.normal_texture.has_value()) {
         auto normal_tex = this->import_texture(src_material.normal_texture->index, asset::TexturePurpose::NormalMap);
         if (!normal_tex.has_value()) {
            return std::nullopt;
         }

         if (src_material.pbr_metallic_roughness.metallic_roughness_texture.has_value()) {
            auto metallic_tex = this->import_texture(src_material.pbr_metallic_roughness.metallic_roughness_texture->index,
                                                     asset::TexturePurpose::Metallic, TextureChannel::Blue);
            if (!metallic_tex.has_value()) {
               return std::nullopt;
            }

            auto roughness_tex = this->import_texture(src_material.pbr_metallic_roughness.metallic_roughness_texture->index,
                                                      asset::TexturePurpose::Roughness, TextureChannel::Green);
            if (!roughness_tex.has_value()) {
               return std::nullopt;
            }

            render_objects::MTProperties_FullPBR properties{
               .texture = *albedo_tex,
               .normal = *normal_tex,
               .roughness = *roughness_tex,
               .metallic = *metallic_tex,
            };
            dst_material.material_template = render_objects::MaterialTemplate::FullPBR;
            dst_material.properties = properties;

         } else {
            render_objects::MTProperties_NormalMap properties{
               .albedo = *albedo_tex,
               .normal = *normal_tex,
               .roughness = src_material.pbr_metallic_roughness.roughness_factor,
               .metallic = src_material.pbr_metallic_roughness.metallic_factor,
            };
            dst_material.material_template = render_objects::MaterialTemplate::NormalMap;
            dst_material.properties = properties;
         }
      } else {
         render_objects::MTProperties_Basic properties{
            .albedo = *albedo_tex,
            .roughness = src_material.pbr_metallic_roughness.roughness_factor,
            .metallic = src_material.pbr_metallic_roughness.metallic_factor,
         };
         dst_material.properties = properties;
         dst_material.material_template = render_objects::MaterialTemplate::Basic;
      }

      auto material_name_str =
         src_material.name.empty() ? std::format("{}.mat{}", strip_extension(m_props.src_path.basename()), material_id) : src_material.name;
      for (char& ch : material_name_str) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto import_path = m_project_info.default_import_path(ResourceType::Material, material_name_str);
      const auto rc_name = make_rc_name(import_path);
      assert(rc_name.type() == ResourceType::Material);

      const auto dst_path = m_project_info.content_path(import_path);
      if (!m_props.should_override && dst_path.exists()) {
         std::print(stderr, "triglav-cli: Failed to import material to {}, file exists\n", dst_path.string());
         return std::nullopt;
      }

      const auto file = io::open_file(dst_path, io::FileMode::Write | io::FileMode::Create);
      if (!file.has_value()) {
         return std::nullopt;
      }

      ryml::Tree tree;
      ryml::NodeRef tree_ref{tree};
      tree_ref |= ryml::MAP;
      dst_material.serialize_yaml(tree_ref);

      const auto str = ryml::emitrs_yaml<std::string>(tree);
      if (!(*file)->write({reinterpret_cast<const u8*>(str.data()), str.size()}).has_value()) {
         return std::nullopt;
      }

      std::print(stderr, "triglav-cli: Imported material to {}\n", import_path);

      if (!add_resource_to_index(import_path)) {
         return std::nullopt;
      }

      m_imported_materials.emplace(material_id, rc_name.name());

      return rc_name;
   }

   [[nodiscard]] std::optional<MeshName> import_mesh(const std::optional<std::string>& name, const u32 mesh_id)
   {
      if (m_imported_meshes.contains(mesh_id)) {
         return m_imported_meshes.at(mesh_id);
      }

      auto mesh_name_str = name.value_or(std::format("{}.mesh{}", strip_extension(m_props.src_path.basename()), mesh_id));
      for (char& ch : mesh_name_str) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto import_path = m_project_info.default_import_path(ResourceType::Mesh, mesh_name_str);
      const auto rc_name = make_rc_name(import_path);
      assert(rc_name.type() == ResourceType::Mesh);

      const auto dst_path = m_project_info.content_path(import_path);
      if (!m_props.should_override && dst_path.exists()) {
         std::print(stderr, "triglav-cli: Failed to import mesh to {}, file exists\n", dst_path.string());
         return std::nullopt;
      }

      const auto& mesh = m_glb_file.document->meshes.at(mesh_id);
      for (const auto& prim : mesh.primitives) {
         if (!prim.material.has_value())
            continue;
         if (!this->import_material(*prim.material).has_value())
            return std::nullopt;
      }

      auto gltf_mesh = gltf::mesh_from_document(*m_glb_file.document, mesh_id, m_glb_file.buffer_manager, m_imported_materials);
      write_mesh_to_file(gltf_mesh, dst_path);
      m_imported_meshes.emplace(mesh_id, rc_name);

      std::print(stderr, "triglav-cli: Importing mesh to {}\n", import_path);

      if (!add_resource_to_index(import_path)) {
         return std::nullopt;
      }

      return rc_name;
   }

   [[nodiscard]] bool import_scene()
   {
      if (!m_props.should_override && m_props.dst_path.exists()) {
         std::print(stderr, "triglav-cli: Failed to import scene, file exists");
         return false;
      }

      const auto& glb_scene = m_glb_file.document->scenes[m_glb_file.document->scene];

      world::LevelNode root_node("root");
      for (const u32 node_id : glb_scene.nodes) {
         auto& glb_node = m_glb_file.document->nodes[node_id];

         if (!glb_node.mesh.has_value())
            continue;

         auto transform = Transform3D::identity();
         if (glb_node.matrix.has_value()) {
            transform = Transform3D::from_matrix(glb_node.matrix.value());
         }
         if (glb_node.rotation.has_value()) {
            transform.rotation = {glb_node.rotation->w, glb_node.rotation->x, glb_node.rotation->y, glb_node.rotation->z};
         }
         if (glb_node.translation.has_value()) {
            transform.translation = glb_node.translation.value();
         }
         if (glb_node.scale.has_value()) {
            transform.scale = glb_node.scale.value();
         }

         auto mesh_name = this->import_mesh(glb_node.name, *glb_node.mesh);
         if (!mesh_name.has_value()) {
            return false;
         }

         world::StaticMesh mesh;
         mesh.name = glb_node.name.value_or(std::format("static_mesh{}", node_id));
         mesh.transform = transform;
         mesh.mesh_name = *mesh_name;

         root_node.add_static_mesh(std::move(mesh));
      }

      world::Level level;
      level.add_node("root"_name, std::move(root_node));

      if (!level.save_to_file(m_props.dst_path)) {
         std::print(stderr, "triglav-cli: Failed to save level file\n");
         return false;
      }

      return true;
   }

 private:
   LevelImportProps m_props;
   project::ProjectInfo m_project_info;
   gltf::GlbResource m_glb_file;
   io::Path m_glb_source_path;
   std::map<u32, MeshName> m_imported_meshes;
   std::map<u32, MaterialName> m_imported_materials;
   std::map<std::pair<u32, std::optional<TextureChannel>>, TextureName> m_imported_textures;
};

bool import_level(const LevelImportProps& props)
{
   std::print(stderr, "triglav-cli: Importing level to {}\n", props.dst_path.string());

   auto project_info = project::load_active_project_info();
   if (!project_info.has_value()) {
      std::print(stderr, "triglav-cli: Failed to load project info\n");
      return false;
   }

   auto glb_file = gltf::open_glb_file(props.src_path);
   if (!glb_file.has_value()) {
      std::print(stderr, "triglav-cli: Failed to load GLB file\n");
      return false;
   }

   LevelImporter importer(props, std::move(*project_info), std::move(*glb_file), props.src_path);
   if (!importer.import_scene()) {
      return false;
   }

   return true;
}

}// namespace triglav::tool::cli
