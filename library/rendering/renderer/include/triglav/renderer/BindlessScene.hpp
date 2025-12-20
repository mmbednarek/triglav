#pragma once

#include "../../../../render_core/include/triglav/render_core/IRenderer.hpp"
#include "Scene.hpp"

#include "triglav/Int.hpp"
#include "triglav/UpdateList.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/memory/HeapAllocator.hpp"
#include "triglav/render_core/RenderCore.hpp"

#include <vector>

namespace triglav::renderer {

struct BindlessSceneObject
{
   u32 index_count{};
   u32 instance_count{};
   u32 index_offset{};
   u32 vertex_offset{};
   u32 instance_offset{};
   u32 material_id{};
   alignas(16) glm::mat4 transform{};
   alignas(16) glm::mat4 normal_transform{};
   alignas(16) glm::vec3 bounding_box_min{};
   alignas(16) glm::vec3 bounding_box_max{};
};

struct BindlessMeshInfo
{
   u32 index_count{};
   u32 index_offset{};
   u32 vertex_offset{};
   u32 material_id{};
   glm::vec3 bounding_box_min{};
   glm::vec3 bounding_box_max{};
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

class BindlessScene
{
   friend class DrawCallUpdateWriter;

 public:
   using Self = BindlessScene;

   BindlessScene(graphics_api::Device& device, resource::ResourceManager& resource_manager, Scene& scene, render_core::IRenderer& renderer);

   void on_object_added_to_scene(ObjectID object_id, const SceneObject& object);
   void on_object_changed_transform(ObjectID object_id, const Transform3D& transform);
   void on_object_removed(ObjectID object_id);
   void on_update_scene(const graphics_api::CommandList& cmd_list);

   void write_objects_to_buffer();

   [[nodiscard]] graphics_api::Buffer& combined_vertex_buffer();
   [[nodiscard]] graphics_api::Buffer& combined_index_buffer();
   [[nodiscard]] graphics_api::Buffer& scene_object_buffer();
   [[nodiscard]] graphics_api::Buffer& material_template_properties(u32 material_template_id);
   [[nodiscard]] const graphics_api::Buffer& count_buffer() const;
   [[nodiscard]] u32 scene_object_count() const;
   [[nodiscard]] Scene& scene() const;
   [[nodiscard]] std::vector<const graphics_api::Texture*>& scene_textures();
   [[nodiscard]] std::vector<render_core::TextureRef>& scene_texture_refs();

 private:
   const std::vector<BindlessMeshInfo>& get_mesh_infos(const graphics_api::CommandList& cmd_list, MeshName name);
   u32 get_material_id(const graphics_api::CommandList& cmd_list, const render_objects::Material& material);
   u32 get_texture_id(TextureName texture_name);

   // References
   resource::ResourceManager& m_resource_manager;
   Scene& m_scene;
   render_core::IRenderer& m_renderer;
   graphics_api::Device& m_device;

   // Caches and temporary buffers
   std::vector<std::pair<ObjectID, Transform3D>> m_pending_transform;
   std::map<MeshName, std::vector<BindlessMeshInfo>> m_models;
   std::map<TextureName, u32> m_texture_ids;
   std::vector<const graphics_api::Texture*> m_scene_textures;
   std::vector<render_core::TextureRef> m_scene_texture_refs;
   std::optional<graphics_api::Pipeline> m_scene_pipeline;
   bool m_should_write_objects{false};
   memory::HeapAllocator m_vertex_buffer_heap;
   memory::HeapAllocator m_index_buffer_heap;
   UpdateList<std::pair<ObjectID, u32>, PendingObject> m_draw_call_update_list;

   // GPU Buffers
   graphics_api::StagingArray<BindlessSceneObject> m_scene_object_stage;
   graphics_api::StagingArray<Matrix4x4> m_transform_stage;
   graphics_api::StorageArray<BindlessSceneObject> m_scene_objects;
   graphics_api::VertexArray<geometry::Vertex> m_combined_vertex_buffer;
   graphics_api::IndexArray m_combined_index_buffer;
   graphics_api::UniformBuffer<u32> m_count_buffer;
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