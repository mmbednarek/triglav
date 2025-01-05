#include "SyncBuffers.hpp"

#include "Geometry.hpp"

namespace triglav::renderer::node {

namespace gapi = graphics_api;
using namespace name_literals;

SyncBuffers::SyncBuffers(Scene& scene) :
    m_scene(scene)
{
}

gapi::WorkTypeFlags SyncBuffers::work_types() const
{
   return gapi::WorkType::Transfer;
}

void SyncBuffers::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& /*resources*/,
                                  graphics_api::CommandList& cmdList)
{
   auto& geometryResources = dynamic_cast<IGeometryResources&>(frameResources.node("geometry"_name));
   auto& ubo = geometryResources.ground_ubo();

   GroundRenderer::prepare_resources(cmdList, ubo, m_scene.camera());
}

std::unique_ptr<render_core::NodeFrameResources> SyncBuffers::create_node_resources()
{
   return std::make_unique<render_core::NodeFrameResources>();
}

}// namespace triglav::renderer::node
