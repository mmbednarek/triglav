#pragma once

#include "Core.h"
#include "graphics_api/CommandList.h"
#include "graphics_api/Pipeline.h"
#include "graphics_api/Shader.h"
#include "graphics_api/Texture.h"

namespace renderer {

class Renderer;

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
   graphics_api::Shader m_fragmentShader;
   graphics_api::Shader m_vertexShader;
   graphics_api::Texture m_texture;
   GpuMesh m_mesh;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorGroup m_descGroup;
   graphics_api::Buffer m_uniformBuffer;
   graphics_api::MappedMemory m_uniformBufferMapping;
};


}// namespace renderer