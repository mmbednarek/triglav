#pragma once

#include "Scene.hpp"

#include "triglav/Int.hpp"
#include "triglav/UpdateList.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/memory/HeapAllocator.hpp"
#include "triglav/render_core/IRenderer.hpp"
#include "triglav/render_core/RenderCore.hpp"

#include <vector>

namespace triglav::renderer {

struct BindlessSceneObject
{
   u32 index_count{};
   u32 index_offset{};
   u32 vertex_offset{};
   u32 material_id{};
   u32 transform_id{};
   u32 matrix_offset{};
   u32 padding[2];
   geometry::BoundingBox bounding_box{};
};

static_assert(sizeof(BindlessSceneObject) % 16 == 0);

struct DrawCall
{
   u32 index_count;
   u32 instance_count;
   u32 index_offset;
   u32 vertex_offset;
   u32 instance_offset;
   u32 material_id;
   u32 matrix_offset;
   u32 padding;
   Matrix4x4 transform;
   Matrix4x4 normal_transform;
};

static_assert(sizeof(DrawCall) % 16 == 0);

struct BindlessMeshInfo
{
   u32 index_count{};
   u32 index_offset{};
   u32 vertex_offset{};
   u32 material_id{};
   geometry::BoundingBox bounding_box{};
};

enum class BindlessValueSource
{
   TextureMap,
   Scalar,
};

struct Properties_MT0
{
   u32 albedo_texture_id{};
   float roughness;
   float metallic;
};

struct Properties_MT1
{
   u32 albedo_texture_id{};
   u32 normal_texture_id{};
   float roughness;
   float metallic;
};

struct Properties_MT2
{
   u32 albedo_texture_id{};
   u32 normal_texture_id{};
   u32 roughness_texture_id{};
   u32 metallic_texture_id{};
};

struct PendingObject
{
   const SceneObject* object{};
   ObjectID object_id{};
   u32 material_index{};
};

struct OffsetCount
{
   u32 offset;
   u32 count;
};

class BindlessScene
{
   TG_DEFINE_LOG_CATEGORY(BindlessScene)

   friend class DrawCallUpdateWriter;

 public:
   using Self = BindlessScene;

   BindlessScene(graphics_api::Device& device, resource::ResourceManager& resource_manager, Scene& scene, render_core::IRenderer& renderer);

   void on_object_added_to_scene(ObjectID object_id, const SceneObject& object);
   void on_object_changed_transform(ObjectID object_id, const Transform3D& transform);
   void on_object_removed(ObjectID object_id);
   void on_update_scene(const graphics_api::CommandList& cmd_list);

   void write_objects_to_buffer();

   [[nodiscard]] u32 transform_id(ObjectID id, u32 transform_index = 0) const;

   [[nodiscard]] graphics_api::Buffer& combined_vertex_buffer();
   [[nodiscard]] graphics_api::Buffer& combined_index_buffer();
   [[nodiscard]] graphics_api::Buffer& scene_object_buffer();
   [[nodiscard]] graphics_api::Buffer& material_template_properties(u32 material_template_id);
   [[nodiscard]] const graphics_api::Buffer& count_buffer() const;
   [[nodiscard]] const graphics_api::Buffer& transform_offset_count_buffer() const;
   [[nodiscard]] const graphics_api::Buffer& transform_buffer() const;
   [[nodiscard]] const graphics_api::Buffer& transform_matrix_buffer() const;
   [[nodiscard]] const graphics_api::Buffer& matrix_hierarchy_buffer() const;
   [[nodiscard]] const graphics_api::Buffer& matrix_hierarchy_count_buffer() const;
   [[nodiscard]] memory::Area transform_allocated_area() const;
   [[nodiscard]] u32 scene_object_count() const;
   [[nodiscard]] Scene& scene() const;
   [[nodiscard]] std::vector<const graphics_api::Texture*>& scene_textures();
   [[nodiscard]] std::vector<render_core::TextureRef>& scene_texture_refs();
   [[nodiscard]] u32 matrix_hierarchy_count() const;

 private:
   u32 get_transform_id(const graphics_api::CommandList& cmd_list, ObjectID object_id, Transform3D* stage_ptr,
                        const Transform3D& transform);
   const std::vector<BindlessMeshInfo>& get_mesh_infos(const graphics_api::CommandList& cmd_list, MeshName name, bool is_skeletal_mesh);
   u32 get_material_id(const graphics_api::CommandList& cmd_list, const render_objects::Material& material);
   u32 get_texture_id(TextureName texture_name);

   // References
   resource::ResourceManager& m_resource_manager;
   Scene& m_scene;
   render_core::IRenderer& m_renderer;
   graphics_api::Device& m_device;

   // Caches and temporary buffers
   std::vector<std::pair<ObjectID, Transform3D>> m_pending_transform;
   std::vector<std::pair<ObjectID, ArmatureName>> m_pending_armatures;
   std::map<MeshName, std::vector<BindlessMeshInfo>> m_models;
   std::map<TextureName, u32> m_texture_ids;
   std::vector<const graphics_api::Texture*> m_scene_textures;
   std::vector<render_core::TextureRef> m_scene_texture_refs;
   std::optional<graphics_api::Pipeline> m_scene_pipeline;
   bool m_should_write_objects{false};
   memory::HeapAllocator m_vertex_buffer_heap;
   memory::HeapAllocator m_index_buffer_heap;
   memory::HeapAllocator m_transform_buffer_heap;
   UpdateList<std::pair<ObjectID, u32>, PendingObject> m_draw_call_update_list;
   std::map<ObjectID, u32> m_object_id_to_transform_id;
   std::map<ObjectID, MemorySize> m_transform_offsets;
   u32 m_transform_stage_index{0};
   u32 m_hierarchy_stage_index{0};
   u32 m_written_hierarchy_count{0};

   // GPU Buffers
   graphics_api::StagingArray<BindlessSceneObject> m_scene_object_stage;
   graphics_api::StagingArray<Transform3D> m_transform_stage;
   graphics_api::StagingArray<Matrix4x4> m_matrix_stage;
   graphics_api::StorageArray<BindlessSceneObject> m_scene_objects;
   graphics_api::Buffer m_combined_vertex_buffer;
   graphics_api::IndexArray m_combined_index_buffer;
   graphics_api::UniformBuffer<u32> m_count_buffer;
   graphics_api::UniformBuffer<OffsetCount> m_transform_offset_count_buffer;
   graphics_api::Buffer m_transform_buffer;
   graphics_api::Buffer m_transform_matrix_buffer;
   graphics_api::Buffer m_hierarchy_stage;
   graphics_api::Buffer m_hierarchy_buffer;
   graphics_api::Buffer m_hierarchy_count_buffer;
   graphics_api::StorageArray<Properties_MT0> m_material_props_albedo_tex;
   graphics_api::StorageArray<Properties_MT1> m_material_props_albedo_normal_tex;
   graphics_api::StorageArray<Properties_MT2> m_material_props_all_tex;

   // Buffer write counts
   MemorySize m_written_material_property_AlbedoTex{0};
   MemorySize m_written_material_property_AlbedoNormalTex{0};
   MemorySize m_written_material_property_AllTex{0};

   // Sinks
   TG_SINK(Scene, OnObjectAddedToScene);
   TG_SINK(Scene, OnObjectChangedTransform);
   TG_SINK(Scene, OnObjectRemoved);
};

}// namespace triglav::renderer