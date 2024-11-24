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
   alignas(16) glm::mat4 transform{};
   alignas(16) glm::mat4 normalTransform{};
   alignas(16) glm::vec3 boundingBoxMin{};
   alignas(16) glm::vec3 boundingBoxMax{};
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

enum class BindlessValueSource
{
   TextureMap,
   Scalar,
};

template<BindlessValueSource Source, typename TScalarType = float>
using BindlessValueType = std::conditional_t<Source == BindlessValueSource::TextureMap, u32, TScalarType>;

template<BindlessValueSource Albedo, BindlessValueSource Roughness, BindlessValueSource Metalic>
struct BindlessMaterialProperties
{
   using enum BindlessValueSource;

   BindlessValueType<Albedo, glm::vec3> albedo{};
   BindlessValueType<Roughness> roughness{};
   BindlessValueType<Metalic> metalic{};
};

using BindlessMaterialProps_AllScalar =
   BindlessMaterialProperties<BindlessValueSource::Scalar, BindlessValueSource::Scalar, BindlessValueSource::Scalar>;
using BindlessMaterialProps_AlbedoTex =
   BindlessMaterialProperties<BindlessValueSource::TextureMap, BindlessValueSource::Scalar, BindlessValueSource::Scalar>;

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
   [[nodiscard]] u32 scene_object_count() const;
   [[nodiscard]] Scene& scene() const;

 private:
   BindlessMeshInfo& get_mesh_info(const graphics_api::CommandList& cmdList, ModelName name);
   u32 get_material_id(const graphics_api::CommandList& cmdList, const render_core::Material& material);
   u32 get_texture_id(TextureName texture);

   // References
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   Scene& m_scene;

   // Caches and temporary buffers
   std::vector<SceneObject> m_pendingObjects;
   std::map<ModelName, BindlessMeshInfo> m_models;
   std::map<TextureName, u32> m_textureIds;

   // GPU Buffers
   graphics_api::StagingArray<BindlessSceneObject> m_sceneObjectStage;
   graphics_api::StorageArray<BindlessSceneObject> m_sceneObjects;
   graphics_api::VertexArray<geometry::Vertex> m_combinedVertexBuffer;
   graphics_api::IndexArray m_combinedIndexBuffer;
   graphics_api::UniformBuffer<u32> m_countBuffer;
   graphics_api::StorageArray<BindlessMaterialProps_AllScalar> m_materialPropsAllScalar;
   graphics_api::StorageArray<BindlessMaterialProps_AlbedoTex> m_materialPropsAlbedoTex;

   // Buffer write counts
   MemorySize m_writtenSceneObjectCount{0};
   MemorySize m_writtenObjectCount{0};
   MemorySize m_writtenVertexCount{0};
   MemorySize m_writtenIndexCount{0};
   MemorySize m_writtenMaterialProperty_AllScalar{0};

   // Sinks
   TG_SINK(Scene, OnObjectAddedToScene);
};

}// namespace triglav::renderer