#include "BindlessScene.hpp"

#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;
using namespace name_literals;

constexpr auto STAGING_BUFFER_ELEM_COUNT = 128;
constexpr auto SCENE_ELEM_COUNT = 256;
constexpr auto VERTEX_BUFFER_SIZE = 1 << 20;
constexpr auto INDEX_BUFFER_SIZE = 1 << 20;

namespace {

constexpr auto encode_material_id(const u32 template_id, const u32 instance_id)
{
   return (template_id & 0b111) | (instance_id << 3);
}

}// namespace

class DrawCallUpdateWriter
{
 public:
   DrawCallUpdateWriter(BindlessScene& bindless_scene, const gapi::CommandList& cmd_list, gapi::Buffer& staging_buffer,
                        gapi::Buffer& dst_buffer, BindlessSceneObject* staging_ptr) :
       m_bindless_scene(bindless_scene),
       m_cmd_list(cmd_list),
       m_staging_buffer(staging_buffer),
       m_dst_buffer(dst_buffer),
       m_staging_ptr(staging_ptr)
   {
   }

   void set_object(const u32 dst, const PendingObject& pending_object)
   {
      const auto& mesh_info = m_bindless_scene.get_mesh_infos(m_cmd_list, pending_object.object->model)[pending_object.material_index];

      BindlessSceneObject bso;
      bso.vertex_offset = mesh_info.vertex_offset;
      bso.index_count = mesh_info.index_count;
      bso.index_offset = mesh_info.index_offset;
      bso.instance_count = 1;
      bso.instance_offset = 0;
      bso.bounding_box_min = mesh_info.bounding_box_min;
      bso.bounding_box_max = mesh_info.bounding_box_max;
      bso.material_id = mesh_info.material_id;
      bso.transform = pending_object.object->model_matrix();
      bso.normal_transform = glm::transpose(glm::inverse(glm::mat3(pending_object.object->model_matrix())));

      m_staging_ptr[m_top_staging_index] = bso;
      m_cmd_list.copy_buffer(m_staging_buffer, m_dst_buffer, m_top_staging_index * sizeof(BindlessSceneObject),
                             dst * sizeof(BindlessSceneObject), sizeof(BindlessSceneObject));
      ++m_top_staging_index;
   }

   void move_object(const u32 src, const u32 dst) const
   {
      m_cmd_list.copy_buffer(m_dst_buffer, m_dst_buffer, src * sizeof(BindlessSceneObject), dst * sizeof(BindlessSceneObject),
                             sizeof(BindlessSceneObject));
   }

 private:
   BindlessScene& m_bindless_scene;
   const gapi::CommandList& m_cmd_list;
   gapi::Buffer& m_staging_buffer;
   gapi::Buffer& m_dst_buffer;
   BindlessSceneObject* m_staging_ptr;
   u32 m_top_staging_index = 0;
};

BindlessScene::BindlessScene(gapi::Device& device, resource::ResourceManager& resource_manager, Scene& scene,
                             render_core::IRenderer& renderer) :
    m_resource_manager(resource_manager),
    m_scene(scene),
    m_renderer(renderer),
    m_device(device),
    m_vertex_buffer_heap(VERTEX_BUFFER_SIZE),
    m_index_buffer_heap(INDEX_BUFFER_SIZE),
    m_scene_object_stage(device, STAGING_BUFFER_ELEM_COUNT),
    m_transform_stage(device, STAGING_BUFFER_ELEM_COUNT),
    m_scene_objects(device, SCENE_ELEM_COUNT, gapi::BufferUsage::Indirect | gapi::BufferUsage::TransferSrc),
    m_combined_vertex_buffer(device, VERTEX_BUFFER_SIZE),
    m_combined_index_buffer(device, INDEX_BUFFER_SIZE),
    m_count_buffer(device, gapi::BufferUsage::Indirect),
    m_material_props_albedo_tex(device, 100),
    m_material_props_albedo_normal_tex(device, 100),
    m_material_props_all_tex(device, 100),
    TG_CONNECT(scene, OnObjectAddedToScene, on_object_added_to_scene),
    TG_CONNECT(scene, OnObjectChangedTransform, on_object_changed_transform),
    TG_CONNECT(scene, OnObjectRemoved, on_object_removed)
{
   TG_SET_DEBUG_NAME(m_scene_objects.buffer(), "bindless_scene.scene_objects");
   TG_SET_DEBUG_NAME(m_scene_object_stage.buffer(), "bindless_scene.scene_objects.staging");
   TG_SET_DEBUG_NAME(m_transform_stage.buffer(), "bindless_scene.object_transform.staging");
   TG_SET_DEBUG_NAME(m_count_buffer.buffer(), "bindless_scene.count_buffer");
   TG_SET_DEBUG_NAME(m_combined_index_buffer.buffer(), "bindless_scene.combined_index_buffer");
   TG_SET_DEBUG_NAME(m_combined_vertex_buffer.buffer(), "bindless_scene.combined_vertex_buffer");
}

void BindlessScene::on_object_added_to_scene(const ObjectID object_id, const SceneObject& object)
{
   const auto& model = m_resource_manager.get(object.model);
   for (u32 i = 0; i < model.range.size(); ++i) {
      m_draw_call_update_list.add_or_update(std::make_pair(object_id, i), PendingObject{&object, object_id, i});
   }
   m_should_write_objects = true;
}

void BindlessScene::on_object_changed_transform(const ObjectID object_id, const Transform3D& transform)
{
   m_pending_transform.emplace_back(object_id, transform);
   m_should_write_objects = true;
}

void BindlessScene::on_object_removed(const ObjectID object_id)
{
   auto& obj = m_scene.object(object_id);
   const auto& model = m_resource_manager.get(obj.model);

   for (u32 i = 0; i < model.range.size(); ++i) {
      m_draw_call_update_list.remove(std::make_pair(object_id, i));
   }
   m_should_write_objects = true;
}

void BindlessScene::on_update_scene(const gapi::CommandList& cmd_list)
{
   const auto object_mapping{GAPI_CHECK(m_scene_object_stage.buffer().map_memory())};
   DrawCallUpdateWriter writer(*this, cmd_list, m_scene_object_stage.buffer(), m_scene_objects.buffer(),
                               static_cast<BindlessSceneObject*>(*object_mapping));
   m_draw_call_update_list.write_to_buffers(writer);

   *m_count_buffer = m_draw_call_update_list.top_index();

   {
      assert(m_pending_transform.size() < STAGING_BUFFER_ELEM_COUNT);
      auto transform_mapping{GAPI_CHECK(m_transform_stage.buffer().map_memory())};

      u32 stage_index = 0;
      for (const auto& [object_id, transform] : m_pending_transform) {
         std::array<Matrix4x4, 2> transforms{
            transform.to_matrix(),
            glm::transpose(glm::inverse(glm::mat3(transform.to_matrix()))),
         };
         transform_mapping.write_offset(transforms.data(), transforms.size() * sizeof(Matrix4x4),
                                        stage_index * transforms.size() * sizeof(Matrix4x4));

         auto& obj = m_scene.object(object_id);
         auto& mesh = m_resource_manager.get(obj.model);
         for (u32 i = 0; i < mesh.range.size(); ++i) {
            auto offset = m_draw_call_update_list.key_map().at(std::make_pair(object_id, i));

            cmd_list.copy_buffer(m_transform_stage.buffer(), m_scene_objects.buffer(),
                                 static_cast<u32>(stage_index * transforms.size() * sizeof(Matrix4x4)),
                                 static_cast<u32>(offset * sizeof(BindlessSceneObject) + offsetof(BindlessSceneObject, transform)),
                                 static_cast<u32>(transforms.size() * sizeof(Matrix4x4)));
         }

         ++stage_index;
      }

      m_pending_transform.clear();
   }
}

void BindlessScene::write_objects_to_buffer()
{
   if (!m_should_write_objects)
      return;
   m_should_write_objects = false;

   const auto copy_objects_cmd_list = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));
   GAPI_CHECK_STATUS(copy_objects_cmd_list.begin());

   this->on_update_scene(copy_objects_cmd_list);

   GAPI_CHECK_STATUS(copy_objects_cmd_list.finish());

   const auto fence = GAPI_CHECK(m_device.create_fence());
   fence.await();

   const graphics_api::SemaphoreArray empty;
   GAPI_CHECK_STATUS(m_device.submit_command_list(copy_objects_cmd_list, empty, empty, &fence, graphics_api::WorkType::Transfer));

   fence.await();
}

gapi::Buffer& BindlessScene::combined_vertex_buffer()
{
   return m_combined_vertex_buffer.buffer();
}

gapi::Buffer& BindlessScene::combined_index_buffer()
{
   return m_combined_index_buffer.buffer();
}

gapi::Buffer& BindlessScene::scene_object_buffer()
{
   return m_scene_objects.buffer();
}

graphics_api::Buffer& BindlessScene::material_template_properties(const u32 material_template_id)
{
   switch (material_template_id) {
   case 0:
      return m_material_props_albedo_tex.buffer();
   case 1:
      return m_material_props_albedo_normal_tex.buffer();
   default:
      return m_material_props_all_tex.buffer();
   }
}

const gapi::Buffer& BindlessScene::count_buffer() const
{
   return m_count_buffer.buffer();
}

u32 BindlessScene::scene_object_count() const
{
   return m_draw_call_update_list.top_index();
}

Scene& BindlessScene::scene() const
{
   return m_scene;
}

std::vector<const graphics_api::Texture*>& BindlessScene::scene_textures()
{
   return m_scene_textures;
}

std::vector<render_core::TextureRef>& BindlessScene::scene_texture_refs()
{
   return m_scene_texture_refs;
}

const std::vector<BindlessMeshInfo>& BindlessScene::get_mesh_infos(const gapi::CommandList& cmd_list, const MeshName name)
{
   if (const auto it = m_models.find(name); it != m_models.end()) {
      return it->second;
   }

   auto& model = m_resource_manager.get(name);

   const auto vertex_offset = m_vertex_buffer_heap.allocate(model.mesh.vertices.count());
   assert(vertex_offset.has_value());

   const auto index_offset = m_index_buffer_heap.allocate(model.mesh.indices.count());
   assert(index_offset.has_value());

   std::vector<BindlessMeshInfo> result;

   assert(!model.range.empty());
   for (const auto& material : model.range) {
      BindlessMeshInfo mesh_info;
      mesh_info.index_count = static_cast<u32>(material.size);
      mesh_info.index_offset = static_cast<u32>(*index_offset + material.offset);
      mesh_info.vertex_offset = static_cast<u32>(*vertex_offset);
      mesh_info.bounding_box_max = model.bounding_box.max;
      mesh_info.bounding_box_min = model.bounding_box.min;
      mesh_info.material_id = this->get_material_id(cmd_list, m_resource_manager.get(material.material_name));
      result.emplace_back(mesh_info);
   }

   auto [emplaced_it, ok] = m_models.emplace(name, std::move(result));
   assert(ok);

   cmd_list.copy_buffer(model.mesh.vertices.buffer(), m_combined_vertex_buffer.buffer(), 0,
                        static_cast<u32>(*vertex_offset * sizeof(geometry::Vertex)),
                        static_cast<u32>(model.mesh.vertices.count() * sizeof(geometry::Vertex)));

   cmd_list.copy_buffer(model.mesh.indices.buffer(), m_combined_index_buffer.buffer(), 0, static_cast<u32>(*index_offset * sizeof(u32)),
                        static_cast<u32>(model.mesh.indices.count() * sizeof(u32)));

   return emplaced_it->second;
}

u32 BindlessScene::get_material_id(const graphics_api::CommandList& cmd_list, const render_objects::Material& material)
{
   switch (material.material_template) {
   case render_objects::MaterialTemplate::Basic: {
      const auto& props = std::get<render_objects::MTProperties_Basic>(material.properties);

      Properties_MT0 albedo_tex;
      albedo_tex.albedo_texture_id = this->get_texture_id(props.albedo);
      albedo_tex.roughness = props.roughness;
      albedo_tex.metallic = props.metallic;

      const auto out_index = static_cast<u32>(m_written_material_property_AlbedoTex);

      cmd_list.update_buffer(m_material_props_albedo_tex.buffer(),
                             static_cast<u32>(m_written_material_property_AlbedoTex * sizeof(Properties_MT0)), sizeof(Properties_MT0),
                             &albedo_tex);

      ++m_written_material_property_AlbedoTex;

      return encode_material_id(0, out_index);
   }
   case render_objects::MaterialTemplate::NormalMap: {
      const auto& props = std::get<render_objects::MTProperties_NormalMap>(material.properties);

      Properties_MT1 albedo_normal_tex;
      albedo_normal_tex.albedo_texture_id = this->get_texture_id(props.albedo);
      albedo_normal_tex.normal_texture_id = this->get_texture_id(props.normal);
      albedo_normal_tex.roughness = props.roughness;
      albedo_normal_tex.metallic = props.metallic;

      const auto out_index = static_cast<u32>(m_written_material_property_AlbedoNormalTex);

      cmd_list.update_buffer(m_material_props_albedo_normal_tex.buffer(),
                             static_cast<u32>(m_written_material_property_AlbedoNormalTex * sizeof(Properties_MT1)), sizeof(Properties_MT1),
                             &albedo_normal_tex);

      ++m_written_material_property_AlbedoNormalTex;

      return encode_material_id(1, out_index);
   }
   case render_objects::MaterialTemplate::FullPBR: {
      const auto& props = std::get<render_objects::MTProperties_FullPBR>(material.properties);

      Properties_MT2 all_tex;
      all_tex.albedo_texture_id = this->get_texture_id(props.texture);
      all_tex.normal_texture_id = this->get_texture_id(props.normal);
      all_tex.roughness_texture_id = this->get_texture_id(props.roughness);
      all_tex.metallic_texture_id = this->get_texture_id(props.metallic);

      const auto out_index = static_cast<u32>(m_written_material_property_AllTex);

      cmd_list.update_buffer(m_material_props_all_tex.buffer(),
                             static_cast<u32>(m_written_material_property_AllTex * sizeof(Properties_MT2)), sizeof(Properties_MT2),
                             &all_tex);

      ++m_written_material_property_AllTex;

      return encode_material_id(2, out_index);
   }
   }

   return 0;
}

u32 BindlessScene::get_texture_id(const TextureName texture_name)
{
   const auto it = m_texture_ids.find(texture_name);
   if (it != m_texture_ids.end()) {
      return it->second;
   }

   m_renderer.recreate_render_jobs();

   auto& texture = m_resource_manager.get(texture_name);
   const auto texture_id = static_cast<u32>(m_scene_textures.size());
   m_scene_textures.emplace_back(&texture);
   m_scene_texture_refs.emplace_back(texture_name);

   m_texture_ids.emplace(texture_name, texture_id);
   return texture_id;
}

}// namespace triglav::renderer
