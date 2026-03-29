#include "LevelImport.hpp"

#include "MeshImport.hpp"
#include "TextureImport.hpp"
#include "triglav/ResourcePathMap.hpp"

#include "triglav/gltf/Glb.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/json_util/Serialize.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"
#include "triglav/render_objects/Armature.hpp"
#include "triglav/render_objects/Material.hpp"
#include "triglav/world/Level.hpp"

#include <print>
#include <queue>
#include <ryml.hpp>
#include <set>
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

constexpr auto MILLISECOND_MULTIPLIER = 1000.0f;

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

asset::AnimationChannelType channel_type_from_path(const std::string_view path)
{
   if (path == "translation")
      return asset::AnimationChannelType::Translation;
   if (path == "rotation")
      return asset::AnimationChannelType::Rotation;
   if (path == "scale")
      return asset::AnimationChannelType::Scale;

   return {};
}

Transform3D transform_from_node(const gltf::Node& node)
{
   auto transform = Transform3D::identity();
   if (node.matrix.has_value()) {
      transform = Transform3D::from_matrix(node.matrix.value());
   }
   if (node.rotation.has_value()) {
      transform.rotation = {node.rotation->w, node.rotation->x, node.rotation->y, node.rotation->z};
   }
   if (node.translation.has_value()) {
      transform.translation = node.translation.value();
   }
   if (node.scale.has_value()) {
      transform.scale = node.scale.value();
   }
   return transform;
}

}// namespace

class LevelImporter
{
 public:
   LevelImporter(LevelImportProps props, gltf::GlbResource&& glb_resource, const io::Path& glb_src_path) :
       m_props(std::move(props)),
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

      const auto [dst_path, rc_name] = project::PathManager::the().import_path(ResourceType::Texture, texture_name_str);

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

      const auto [dst_path, rc_name] = project::PathManager::the().import_path(ResourceType::Material, material_name_str);
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

      std::print(stderr, "triglav-cli: Imported material to {}\n", dst_path.string());

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

      const auto [dst_path, rc_name] = project::PathManager::the().import_path(ResourceType::Mesh, mesh_name_str);
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

      std::print(stderr, "triglav-cli: Importing mesh to {}\n", dst_path.string());

      return rc_name;
   }

   [[nodiscard]] std::optional<AnimationName> import_animation(const u32 animation_id)
   {
      asset::Animation dst_animation{};
      auto& src_animation = m_glb_file.document->animations[animation_id];

      for (const auto& channel : src_animation.channels) {
         asset::AnimationChannel dst_channel{};
         dst_channel.type = channel_type_from_path(channel.target.path);

         const auto& src_sampler = src_animation.samplers.at(channel.sampler);

         const auto& input_accessor = m_glb_file.document->accessors.at(src_sampler.input);
         const auto& output_accessor = m_glb_file.document->accessors.at(src_sampler.output);

         assert(input_accessor.type == gltf::AccessorType::Scalar);
         assert(output_accessor.component_type == gltf::ComponentType::Float);
         assert(input_accessor.component_type == gltf::ComponentType::Float);

         dst_channel.timestamps.resize(input_accessor.count);
         dst_channel.keyframes.resize(output_accessor.count);

         auto input_stream = m_glb_file.buffer_manager.read_buffer_view(input_accessor.buffer_view);
         for (MemorySize i = 0; i < input_accessor.count; ++i) {
            dst_channel.timestamps[i] = input_stream.read_float() * MILLISECOND_MULTIPLIER;
         }

         auto output_stream = m_glb_file.buffer_manager.read_buffer_view(output_accessor.buffer_view);

         switch (output_accessor.type) {
         case gltf::AccessorType::Vector3: {
            for (MemorySize i = 0; i < output_accessor.count; ++i) {
               dst_channel.keyframes[i] = Vector4{output_stream.read_vec3(), 0.0f};
            }
            break;
         }
         case gltf::AccessorType::Vector4: {
            for (MemorySize i = 0; i < output_accessor.count; ++i) {
               dst_channel.keyframes[i] = output_stream.read_vec4();
            }
         }
         default:
            break;
         }

         dst_animation.channels.emplace_back(std::move(dst_channel));
      }

      auto animation_name_str = src_animation.name;
      if (animation_name_str.empty()) {
         animation_name_str = std::format("{}.anim{}", strip_extension(m_props.src_path.basename()), animation_id);
      }
      for (char& ch : animation_name_str) {
         ch = static_cast<char>(std::tolower(ch));
      }

      const auto [dst_path, rc_name] = project::PathManager::the().import_path(ResourceType::Animation, animation_name_str);

      const auto dst_file = io::open_file(dst_path, io::FileMode::Write | io::FileMode::Create);
      if (!dst_file.has_value()) {
         return std::nullopt;
      }

      if (!json_util::serialize(dst_animation.to_meta_ref(), **dst_file)) {
         return std::nullopt;
      }

      std::print(stderr, "triglav-cli: Importing animation to {}\n", dst_path.string());

      return rc_name;
   }

   [[nodiscard]] std::optional<ArmatureName> import_armature(const u32 skin_id)
   {
      if (m_imported_armatures.contains(skin_id)) {
         return m_imported_armatures.at(skin_id);
      }
      const auto& skin = m_glb_file.document->skins[skin_id];

      render_objects::BoneList bone_list{};
      bone_list.bones.resize(skin.joints.size());

      std::map<u32, u32> node_id_to_bone_id;
      MemorySize bone_id = 0;
      for (const auto joint_id : skin.joints) {
         bone_list.bones[bone_id].parent = render_objects::BONE_ID_NO_PARENT;
         node_id_to_bone_id[joint_id] = bone_id;
         ++bone_id;
      }

      bone_id = 0;
      for (const auto joint_id : skin.joints) {
         const auto& joint_node = m_glb_file.document->nodes[joint_id];
         render_objects::Bone& dst_bone = bone_list.bones[bone_id];
         dst_bone.transform = transform_from_node(joint_node);
         for (const auto child_id : joint_node.children) {
            const auto child_bone_id = node_id_to_bone_id[child_id];
            bone_list.bones[child_bone_id].parent = bone_id;
         }
         ++bone_id;
      }

      auto armature_name_str = skin.name.value_or("");
      if (armature_name_str.empty()) {
         armature_name_str = std::format("{}.armature{}", strip_extension(m_props.src_path.basename()), skin_id);
      }
      for (char& ch : armature_name_str) {
         ch = static_cast<char>(std::tolower(ch));
      }

      const auto [dst_path, rc_name] = project::PathManager::the().import_path(ResourceType::Armature, armature_name_str);

      const auto dst_file = io::open_file(dst_path, io::FileMode::Write | io::FileMode::Create);
      if (!dst_file.has_value()) {
         return std::nullopt;
      }

      if (!json_util::serialize(bone_list.to_meta_ref(), **dst_file)) {
         return std::nullopt;
      }

      m_imported_armatures.emplace(skin_id, rc_name);
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
      std::queue<std::pair<u32, Transform3D>> nodes;

      for (const u32 node_id : glb_scene.nodes) {
         nodes.emplace(node_id, Transform3D::identity());
      }

      std::set<u32> visited_nodes;

      while (!nodes.empty()) {
         auto [node_id, parent_transform] = nodes.front();
         nodes.pop();

         if (visited_nodes.contains(node_id))
            continue;

         visited_nodes.insert(node_id);

         const auto& glb_node = m_glb_file.document->nodes[node_id];
         auto transform = parent_transform.combine(transform_from_node(glb_node));

         for (const auto child_id : glb_node.children) {
            nodes.emplace(child_id, transform);
         }

         std::optional<ArmatureName> armature_name;
         if (glb_node.skin.has_value()) {
            armature_name = this->import_armature(*glb_node.skin);
         }

         if (glb_node.mesh.has_value()) {
            auto mesh_name = this->import_mesh(glb_node.name, *glb_node.mesh);
            if (!mesh_name.has_value()) {
               return false;
            }

            world::StaticMesh mesh;
            mesh.name = glb_node.name.value_or(std::format("static_mesh{}", node_id));
            mesh.transform = transform;
            mesh.mesh_name = *mesh_name;
            mesh.armature_name = armature_name;

            root_node.add_static_mesh(std::move(mesh));
         }
      }

      world::Level level;
      level.add_node("root"_name, std::move(root_node));

      // import animations
      for (u32 animation_id = 0; animation_id < m_glb_file.document->animations.size(); ++animation_id) {
         if (!this->import_animation(animation_id).has_value()) {
            std::print(stderr, "triglav-cli: Failed to import animation {}\n", animation_id);
         }
      }

      if (!level.save_to_file(m_props.dst_path)) {
         std::print(stderr, "triglav-cli: Failed to save level file\n");
         return false;
      }

      return true;
   }

 private:
   LevelImportProps m_props;
   gltf::GlbResource m_glb_file;
   io::Path m_glb_source_path;
   std::map<u32, MeshName> m_imported_meshes;
   std::map<u32, MaterialName> m_imported_materials;
   std::map<u32, ArmatureName> m_imported_armatures;
   std::map<std::pair<u32, std::optional<TextureChannel>>, TextureName> m_imported_textures;
};

bool import_level(const LevelImportProps& props)
{
   std::print(stderr, "triglav-cli: Importing level to {}\n", props.dst_path.string());

   auto glb_file = gltf::open_glb_file(props.src_path);
   if (!glb_file.has_value()) {
      std::print(stderr, "triglav-cli: Failed to load GLB file\n");
      return false;
   }

   LevelImporter importer(props, std::move(*glb_file), props.src_path);
   if (!importer.import_scene()) {
      return false;
   }

   return true;
}

}// namespace triglav::tool::cli
