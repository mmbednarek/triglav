#include "BindlessScene.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/render_objects/Armature.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;
using namespace name_literals;

constexpr auto STAGING_BUFFER_ELEM_COUNT = 128;
constexpr auto SCENE_ELEM_COUNT = 256;
constexpr auto VERTEX_BUFFER_SIZE = 1 << 21;
constexpr auto INDEX_BUFFER_SIZE = 1 << 21;
constexpr auto TRANSFORM_BUFF_COUNT = 1024;
constexpr auto TRANSFORM_MATRIX_BUFF_COUNT = 1024;
constexpr auto HIERARCHY_COUNT = 1024;
constexpr auto MAX_MATRIX_IDS = 18;

namespace {

struct Hierarchy
{
   uint destination_id;
   uint matrix_count;
   uint matrix_ids[MAX_MATRIX_IDS];
};

constexpr auto encode_material_id(const u32 template_id, const u32 instance_id)
{
   return (template_id & 0b111) | (instance_id << 3);
}

}// namespace

class DrawCallUpdateWriter
{
 public:
   DrawCallUpdateWriter(BindlessScene& bindless_scene, const gapi::CommandList& cmd_list, gapi::Buffer& staging_buffer,
                        gapi::Buffer& dst_buffer, BindlessSceneObject* staging_ptr, Transform3D* transform_stage_ptr,
                        const std::map<ObjectID, MemorySize>& matrix_offsets) :
       m_bindless_scene(bindless_scene),
       m_cmd_list(cmd_list),
       m_staging_buffer(staging_buffer),
       m_dst_buffer(dst_buffer),
       m_staging_ptr(staging_ptr),
       m_transform_stage_ptr(transform_stage_ptr),
       m_matrix_offsets(matrix_offsets)
   {
   }

   void set_object(const u32 dst, const PendingObject& pending_object)
   {
      u32 matrix_offset = ~0u;
      if (const auto it = m_matrix_offsets.find(pending_object.object_id); it != m_matrix_offsets.end()) {
         matrix_offset = it->second;
      }
      const auto& mesh_info =
         m_bindless_scene.get_mesh_infos(m_cmd_list, pending_object.object->model, matrix_offset != ~0u)[pending_object.material_index];

      BindlessSceneObject bso;
      bso.vertex_offset = mesh_info.vertex_offset;
      bso.index_count = mesh_info.index_count;
      bso.index_offset = mesh_info.index_offset;
      bso.bounding_box = mesh_info.bounding_box;
      bso.material_id = mesh_info.material_id;
      bso.transform_id =
         m_bindless_scene.get_transform_id(m_cmd_list, pending_object.object_id, m_transform_stage_ptr, pending_object.object->transform);
      bso.matrix_offset = matrix_offset;

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
   Transform3D* m_transform_stage_ptr;
   u32 m_top_staging_index = 0;
   const std::map<ObjectID, MemorySize>& m_matrix_offsets;
};

BindlessScene::BindlessScene(gapi::Device& device, resource::ResourceManager& resource_manager, Scene& scene,
                             render_core::IRenderer& renderer) :
    m_resource_manager(resource_manager),
    m_scene(scene),
    m_renderer(renderer),
    m_device(device),
    m_vertex_buffer_heap(VERTEX_BUFFER_SIZE),
    m_index_buffer_heap(INDEX_BUFFER_SIZE),
    m_transform_buffer_heap(TRANSFORM_BUFF_COUNT),
    m_scene_object_stage(device, STAGING_BUFFER_ELEM_COUNT),
    m_transform_stage(device, STAGING_BUFFER_ELEM_COUNT),
    m_matrix_stage(device, STAGING_BUFFER_ELEM_COUNT),
    m_scene_objects(device, SCENE_ELEM_COUNT, gapi::BufferUsage::Indirect | gapi::BufferUsage::TransferSrc),
    m_combined_vertex_buffer(GAPI_CHECK(
       device.create_buffer(graphics_api::BufferUsage::VertexBuffer | graphics_api::BufferUsage::TransferDst, VERTEX_BUFFER_SIZE))),
    m_combined_index_buffer(device, INDEX_BUFFER_SIZE),
    m_count_buffer(device, gapi::BufferUsage::Indirect),
    m_transform_offset_count_buffer(device, gapi::BufferUsage::Indirect),
    m_transform_buffer(GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst,
                                                       TRANSFORM_BUFF_COUNT * sizeof(Transform3D)))),
    m_transform_matrix_buffer(
       GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer, TRANSFORM_BUFF_COUNT * sizeof(Matrix4x4)))),
    m_hierarchy_stage(GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::HostVisible |
                                                         graphics_api::BufferUsage::TransferSrc,
                                                      STAGING_BUFFER_ELEM_COUNT * sizeof(Hierarchy)))),
    m_hierarchy_buffer(GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::TransferDst,
                                                       HIERARCHY_COUNT * sizeof(Hierarchy)))),
    m_hierarchy_count_buffer(
       GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::UniformBuffer | graphics_api::BufferUsage::TransferDst, sizeof(u32)))),
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
   TG_SET_DEBUG_NAME(m_combined_vertex_buffer, "bindless_scene.combined_vertex_buffer");
}

void BindlessScene::on_object_added_to_scene(const ObjectID object_id, const SceneObject& object)
{
   const auto& model = m_resource_manager.get(object.model);
   for (u32 i = 0; i < model.device_mesh.ranges.size(); ++i) {
      m_draw_call_update_list.add_or_update(std::make_pair(object_id, i), PendingObject{&object, object_id, i});
   }
   if (object.armature.has_value()) {
      m_pending_armatures.emplace_back(std::make_pair(object_id, object.armature.value()));
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

   for (u32 i = 0; i < model.device_mesh.ranges.size(); ++i) {
      m_draw_call_update_list.remove(std::make_pair(object_id, i));
   }
   m_should_write_objects = true;
}

void BindlessScene::on_update_scene(const gapi::CommandList& cmd_list)
{
   m_transform_stage_index = 0;
   m_hierarchy_stage_index = 0;

   const auto object_mapping{GAPI_CHECK(m_scene_object_stage.buffer().map_memory())};
   const auto transform_mapping{GAPI_CHECK(m_transform_stage.buffer().map_memory())};
   const auto matrix_mapping{GAPI_CHECK(m_matrix_stage.buffer().map_memory())};
   const auto hierarchy_mapping{GAPI_CHECK(m_hierarchy_stage.map_memory())};

   std::map<ObjectID, MemorySize> matrix_offsets;

   for (const auto& [object_ids, armature_name] : m_pending_armatures) {
      const auto& armature = m_resource_manager.get(armature_name);

      const auto transform_allocation = m_transform_buffer_heap.allocate(3 * armature.bone_count());
      assert(transform_allocation.has_value());

      m_transform_offsets[object_ids] = *transform_allocation;
      matrix_offsets[object_ids] = *transform_allocation + 2 * armature.bone_count();

      cmd_list.copy_buffer(m_transform_stage.buffer(), m_transform_buffer, 0, *transform_allocation * sizeof(Transform3D),
                           2 * armature.bone_count() * sizeof(Transform3D));
      cmd_list.copy_buffer(m_matrix_stage.buffer(), m_transform_matrix_buffer, 0,
                           (*transform_allocation + armature.bone_count()) * sizeof(Matrix4x4), armature.bone_count() * sizeof(Matrix4x4));
      cmd_list.copy_buffer(m_hierarchy_stage, m_hierarchy_buffer, 0, m_written_hierarchy_count * sizeof(Hierarchy),
                           armature.bone_count() * sizeof(Hierarchy));
      m_written_hierarchy_count += armature.bone_count();

      for (MemorySize bone_id = 0; bone_id < armature.bone_count(); ++bone_id) {
         const auto& bone = armature.bone(bone_id);
         transform_mapping.write_offset(&bone.transform, sizeof(Transform3D), m_transform_stage_index * sizeof(Transform3D));
         matrix_mapping.write_offset(&bone.inverse_bind, sizeof(Matrix4x4), m_transform_stage_index * sizeof(Matrix4x4));
         ++m_transform_stage_index;

         std::array<u32, MAX_MATRIX_IDS> matrices{};

         u32 dst_matrix = 0;
         u32 parent_id = bone_id;
         while (parent_id != render_objects::BONE_ID_NO_PARENT) {
            matrices[dst_matrix] = *transform_allocation + parent_id;
            ++dst_matrix;
            parent_id = armature.bone(parent_id).parent;
         }

         assert(dst_matrix < MAX_MATRIX_IDS);

         Hierarchy hierarchy{
            .destination_id = static_cast<u32>(*transform_allocation + 2 * armature.bone_count() + bone_id),
            .matrix_count = dst_matrix + 1,
            .matrix_ids = {},
         };
         for (u32 i = 0; i < dst_matrix; ++i) {
            hierarchy.matrix_ids[i] = matrices[dst_matrix - 1 - i];
         }
         hierarchy.matrix_ids[dst_matrix] = *transform_allocation + armature.bone_count() + bone_id;// inverse bind
         hierarchy_mapping.write_offset(&hierarchy, sizeof(Hierarchy), m_hierarchy_stage_index * sizeof(Hierarchy));
         ++m_hierarchy_stage_index;
      }

      const Transform3D null_transform = Transform3D::null();
      for (MemorySize bone_id = 0; bone_id < armature.bone_count(); ++bone_id) {
         transform_mapping.write_offset(&null_transform, sizeof(Transform3D), m_transform_stage_index * sizeof(Transform3D));
         ++m_transform_stage_index;
      }
   }

   m_pending_armatures.clear();

   GAPI_CHECK_STATUS(m_hierarchy_count_buffer.write_indirect(&m_written_hierarchy_count, sizeof(u32)));

   DrawCallUpdateWriter writer(*this, cmd_list, m_scene_object_stage.buffer(), m_scene_objects.buffer(),
                               static_cast<BindlessSceneObject*>(*object_mapping), static_cast<Transform3D*>(*transform_mapping),
                               matrix_offsets);
   m_draw_call_update_list.write_to_buffers(writer);

   *m_count_buffer = m_draw_call_update_list.top_index();

   {
      assert(m_pending_transform.size() < STAGING_BUFFER_ELEM_COUNT);

      for (const auto& [object_id, transform] : m_pending_transform) {
         transform_mapping.write_offset(&transform, sizeof(Transform3D), m_transform_stage_index * sizeof(Transform3D));

         const auto transform_id = m_object_id_to_transform_id.at(object_id);
         cmd_list.copy_buffer(m_transform_stage.buffer(), m_transform_buffer,
                              static_cast<u32>(m_transform_stage_index * sizeof(Transform3D)),
                              static_cast<u32>(transform_id * sizeof(Transform3D)), sizeof(Transform3D));

         ++m_transform_stage_index;
      }

      m_pending_transform.clear();
   }

   const auto transform_area = m_transform_buffer_heap.allocated_area();
   *m_transform_offset_count_buffer = {.offset = static_cast<u32>(transform_area.offset), .count = static_cast<u32>(transform_area.size)};
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

u32 BindlessScene::transform_id(const ObjectID id, const u32 transform_index) const
{
   if (transform_index == 0) {
      return m_object_id_to_transform_id.at(id);
   }
   return m_transform_offsets.at(id) + transform_index - 1;
}

gapi::Buffer& BindlessScene::combined_vertex_buffer()
{
   return m_combined_vertex_buffer;
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

const graphics_api::Buffer& BindlessScene::transform_offset_count_buffer() const
{
   return m_transform_offset_count_buffer.buffer();
}

const graphics_api::Buffer& BindlessScene::transform_buffer() const
{
   return m_transform_buffer;
}

const graphics_api::Buffer& BindlessScene::transform_matrix_buffer() const
{
   return m_transform_matrix_buffer;
}

const graphics_api::Buffer& BindlessScene::matrix_hierarchy_buffer() const
{
   return m_hierarchy_buffer;
}

const graphics_api::Buffer& BindlessScene::matrix_hierarchy_count_buffer() const
{
   return m_hierarchy_count_buffer;
}

memory::Area BindlessScene::transform_allocated_area() const
{
   return m_transform_buffer_heap.allocated_area();
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

u32 BindlessScene::matrix_hierarchy_count() const
{
   return m_written_hierarchy_count;
}

u32 BindlessScene::get_transform_id(const graphics_api::CommandList& cmd_list, const ObjectID object_id, Transform3D* stage_ptr,
                                    const Transform3D& transform)
{
   const auto transform_dst_index = m_transform_buffer_heap.allocate(1);
   assert(transform_dst_index.has_value());

   stage_ptr[m_transform_stage_index] = transform;

   cmd_list.copy_buffer(m_transform_stage.buffer(), m_transform_buffer, static_cast<u32>(m_transform_stage_index * sizeof(Transform3D)),
                        *transform_dst_index * sizeof(Transform3D), sizeof(Transform3D));
   ++m_transform_stage_index;

   m_object_id_to_transform_id[object_id] = *transform_dst_index;

   return *transform_dst_index;
}

const std::vector<BindlessMeshInfo>& BindlessScene::get_mesh_infos(const gapi::CommandList& cmd_list, const MeshName name,
                                                                   const bool is_skeletal_mesh)
{
   if (const auto it = m_models.find(name); it != m_models.end()) {
      return it->second;
   }

   const auto& model = m_resource_manager.get(name);

   std::vector<BindlessMeshInfo> result;

   // Index head is per u32
   const auto index_offset = m_index_buffer_heap.allocate(model.device_mesh.index_buffer.count());
   assert(index_offset.has_value());

   assert(!model.device_mesh.ranges.empty());
   for (const auto& material : model.device_mesh.ranges) {
      auto components = material.components;
      if (is_skeletal_mesh) {
         components |= geometry::VertexComponent::Skeleton;
      }
      const auto vertex_size = geometry::get_vertex_size(components);

      // Index head is per u8
      const auto vertex_offset = m_vertex_buffer_heap.allocate(material.vertex_size, vertex_size);
      assert(vertex_offset.has_value());
      assert(*vertex_offset % vertex_size == 0);

      cmd_list.copy_buffer(model.device_mesh.vertex_buffer, m_combined_vertex_buffer, material.vertex_offset, *vertex_offset,
                           material.vertex_size);

      BindlessMeshInfo mesh_info;
      mesh_info.index_count = material.index_size;
      mesh_info.index_offset = *index_offset + material.index_offset;
      mesh_info.vertex_offset = static_cast<u32>(*vertex_offset / vertex_size);
      mesh_info.material_id = this->get_material_id(cmd_list, m_resource_manager.get(material.material_name));
      mesh_info.bounding_box = model.bounding_box;
      result.emplace_back(mesh_info);
   }

   auto [emplaced_it, ok] = m_models.emplace(name, std::move(result));
   assert(ok);

   cmd_list.copy_buffer(model.device_mesh.index_buffer.buffer(), m_combined_index_buffer.buffer(), 0,
                        static_cast<u32>(*index_offset * sizeof(u32)),
                        static_cast<u32>(model.device_mesh.index_buffer.count() * sizeof(u32)));

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
