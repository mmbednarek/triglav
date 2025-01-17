#pragma once

#include <map>
#include <optional>

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/RenderTarget.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::renderer {

struct MaterialResources
{
   MaterialTemplateName materialTemplate;
   std::optional<graphics_api::Buffer> constantsUniformBuffer;
   std::optional<graphics_api::Buffer> worldDataUniformBuffer;
   std::vector<TextureName> textures;
};

struct MaterialTemplateResources
{
   graphics_api::Pipeline pipeline;
};

class MaterialManager
{
 public:
   MaterialManager(graphics_api::Device& device, resource::ResourceManager& resourceManager, graphics_api::RenderTarget& renderTarget);

   [[nodiscard]] MaterialResources& material_resources(MaterialName name);
   [[nodiscard]] const MaterialTemplateResources& material_template_resources(MaterialTemplateName name) const;

 private:
   void process_material(MaterialName name, const render_objects::Material& material);
   void process_material_template(MaterialTemplateName name, const render_objects::MaterialTemplate& materialTemplate);

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   graphics_api::RenderTarget& m_renderTarget;
   std::map<MaterialTemplateName, MaterialTemplateResources> m_templates;
   std::map<MaterialName, MaterialResources> m_materials;
};

}// namespace triglav::renderer