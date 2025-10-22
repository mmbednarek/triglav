#pragma once

#include "Scene.hpp"

#include "triglav/Int.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/graphics_api/Array.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/render_core/RenderCore.hpp"

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

struct Properties_MT0
{
   u32 albedoTextureID{};
   float roughness;
   float metallic;
};

struct Properties_MT1
{
   u32 albedoTextureID{};
   u32 normalTextureID{};
   float roughness;
   float metallic;
};

struct Properties_MT2
{
   u32 albedoTextureID{};
   u32 normalTextureID{};
   u32 roughnessTextureID{};
   u32 metallicTextureID{};
};

class BindlessScene
{
 public:
   using Self = BindlessScene;

   BindlessScene(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene);

   void on_object_added_to_scene(const SceneObject& object);
   void on_update_scene(const graphics_api::CommandList& cmdList);

   void write_object_to_buffer();

   [[nodiscard]] graphics_api::Buffer& combined_vertex_buffer();
   [[nodiscard]] graphics_api::Buffer& combined_index_buffer();
   [[nodiscard]] graphics_api::Buffer& scene_object_buffer();
   [[nodiscard]] graphics_api::Buffer& material_template_properties(u32 materialTemplateId);
   [[nodiscard]] const graphics_api::Buffer& count_buffer() const;
   [[nodiscard]] u32 scene_object_count() const;
   [[nodiscard]] Scene& scene() const;
   [[nodiscard]] std::vector<const graphics_api::Texture*>& scene_textures();
   [[nodiscard]] std::vector<render_core::TextureRef>& scene_texture_refs();

 private:
   BindlessMeshInfo& get_mesh_info(const graphics_api::CommandList& cmdList, MeshName name);
   u32 get_material_id(const graphics_api::CommandList& cmdList, const render_objects::Material& material);
   u32 get_texture_id(TextureName textureName);

   // References
   resource::ResourceManager& m_resourceManager;
   Scene& m_scene;
   graphics_api::Device& m_device;

   // Caches and temporary buffers
   std::vector<SceneObject> m_pendingObjects;
   std::map<MeshName, BindlessMeshInfo> m_models;
   std::map<TextureName, u32> m_textureIds;
   std::vector<const graphics_api::Texture*> m_sceneTextures;
   std::vector<render_core::TextureRef> m_sceneTextureRefs;
   std::optional<graphics_api::Pipeline> m_scenePipeline;
   bool m_shouldUpdatePSO{false};

   // GPU Buffers
   graphics_api::StagingArray<BindlessSceneObject> m_sceneObjectStage;
   graphics_api::StorageArray<BindlessSceneObject> m_sceneObjects;
   graphics_api::VertexArray<geometry::Vertex> m_combinedVertexBuffer;
   graphics_api::IndexArray m_combinedIndexBuffer;
   graphics_api::UniformBuffer<u32> m_countBuffer;
   graphics_api::StorageArray<Properties_MT0> m_materialPropsAlbedoTex;
   graphics_api::StorageArray<Properties_MT1> m_materialPropsAlbedoNormalTex;
   graphics_api::StorageArray<Properties_MT2> m_materialPropsAllTex;

   // Buffer write counts
   MemorySize m_writtenSceneObjectCount{0};
   MemorySize m_writtenObjectCount{0};
   MemorySize m_writtenVertexCount{0};
   MemorySize m_writtenIndexCount{0};
   MemorySize m_writtenMaterialProperty_AlbedoTex{0};
   MemorySize m_writtenMaterialProperty_AlbedoNormalTex{0};
   MemorySize m_writtenMaterialProperty_AllTex{0};

   // Sinks
   TG_SINK(Scene, OnObjectAddedToScene);
};

}// namespace triglav::renderer