#pragma once

#include "Core.h"
#include "graphics_api/CommandList.h"
#include "graphics_api/Pipeline.h"
#include "graphics_api/Shader.h"
#include "graphics_api/Texture.h"

namespace renderer {

class Renderer;
class ResourceManager;

struct SkyBoxUBO
{
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

class SkyBox
{
 public:
   explicit SkyBox(Renderer &renderer);

   void on_render(const graphics_api::CommandList& commandList, float yaw, float pitch, float width, float height) const;

 private:
   ResourceManager& m_resourceManager;
   GpuMesh m_mesh;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descPool;
   graphics_api::DescriptorArray m_descArray;
   graphics_api::DescriptorView m_descriptorSet;
   graphics_api::Sampler m_sampler;
   graphics_api::Buffer m_uniformBuffer;
   graphics_api::MappedMemory m_uniformBufferMapping;
};


}// namespace renderer