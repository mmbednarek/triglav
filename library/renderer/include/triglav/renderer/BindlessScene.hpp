#pragma once

#include "Scene.hpp"

#include "triglav/Int.hpp"
#include "triglav/geometry/Geometry.h"
#include "triglav/graphics_api/Array.hpp"

#include <glm/vec3.hpp>
#include <vector>

namespace triglav::renderer {

struct BindlessSceneObject
{
   u32 indexCount{};
   u32 instanceCount{};
   u32 indexOffset{};
   u32 vertexOffset{};
   u32 instanceOffset{};
   u32 materialID{};
   glm::mat4 transform{};
   glm::vec3 boundingBoxMin{};
   glm::vec3 boundingBoxMax{};
};

struct BindlessMeshInfo
{
   u32 indexCount{};
   u32 indexOffset{};
   u32 vertexOffset{};
   u32 materialID{};
   glm::vec3 boundingBoxMin{};
   glm::vec3 boundingBoxMax{};
};

struct SolidColorProperties
{
   glm::vec3 color;
   float metalic;
   float roughness;
};

class BindlessScene
{
 public:
   using Self = BindlessScene;

   BindlessScene(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene);

   void on_object_added_to_scene(const SceneObject& object);
   void on_update_scene(const graphics_api::CommandList& cmdList);

   [[nodiscard]] graphics_api::Buffer& combined_vertex_buffer();
   [[nodiscard]] graphics_api::Buffer& combined_index_buffer();
   [[nodiscard]] graphics_api::Buffer& scene_object_buffer();
   [[nodiscard]] graphics_api::Buffer& material_template_properties(u32 materialTemplateId);
   [[nodiscard]] const graphics_api::Buffer& count_buffer() const;
   [[nodiscard]] Scene& scene() const;

 private:
   BindlessMeshInfo& get_mesh_info(const graphics_api::CommandList& cmdList, ModelName name);

   // References
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   Scene &m_scene;

   // Caches and temporary buffers
   std::vector<SceneObject> m_pendingObjects;
   std::map<ModelName, BindlessMeshInfo> m_models;

   // GPU Buffers
   graphics_api::StagingArray<BindlessSceneObject> m_sceneObjectStage;
   graphics_api::StorageArray<BindlessSceneObject> m_sceneObjects;
   graphics_api::VertexArray<geometry::Vertex> m_combinedVertexBuffer;
   graphics_api::VertexArray<u32> m_combinedIndexBuffer;
   graphics_api::UniformBuffer<u32> m_countBuffer;
   graphics_api::StorageArray<SolidColorProperties> m_solidColorProps;

   // Buffer write counts
   MemorySize m_writtenSceneObjectCount{0};
   MemorySize m_writtenObjectCount{0};
   MemorySize m_writtenVertexCount{0};
   MemorySize m_writtenIndexCount{0};

   // Sinks
   TG_SINK(Scene, OnObjectAddedToScene);
};

}// namespace triglav::renderer