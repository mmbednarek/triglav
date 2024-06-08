#pragma once

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer {

class Renderer;

class SkyBox
{
 public:
   struct UniformData
   {
      alignas(16) glm::mat4 view;
      alignas(16) glm::mat4 proj;
   };
   using UniformBuffer = graphics_api::UniformBuffer<UniformData>;

   explicit SkyBox(graphics_api::Device& device, resource::ResourceManager& resourceManager, graphics_api::RenderTarget& renderTarget);

   void on_render(graphics_api::CommandList& commandList, UniformBuffer& ubo, float yaw, float pitch, float width, float height) const;

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   render_core::GpuMesh m_mesh;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Texture& m_texture;
};


}// namespace triglav::renderer