#include "FrameResources.h"

#include "triglav/graphics_api/Device.h"

#include <ranges>

namespace triglav::render_core {

using namespace name_literals;

void NodeResourcesBase::add_signal_semaphore(NameID child, graphics_api::Semaphore&& semaphore)
{
   m_ownedSemaphores.emplace(child, std::move(semaphore));
}

void NodeResourcesBase::clean(graphics_api::Device &device)
{
   m_ownedSemaphores.clear();
}

graphics_api::Semaphore& NodeResourcesBase::semaphore(NameID child)
{
   return m_ownedSemaphores[child];
}

void NodeResourcesBase::finalize()
{
   m_ownedSemaphores.make_heap();
}

void NodeFrameResources::add_render_target(const NameID identifier, graphics_api::RenderTarget &renderTarget)
{
   m_renderTargets.emplace_back(identifier, &renderTarget, std::nullopt, std::nullopt);
}

void NodeFrameResources::add_render_target_with_resolution(const NameID identifier,
                                                           graphics_api::RenderTarget &renderTarget,
                                                           const graphics_api::Resolution &resolution)
{
   m_renderTargets.emplace_back(identifier, &renderTarget, std::nullopt, resolution);
}

void NodeFrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   // TODO: Change framebuffer when not used.
   for (auto &target : m_renderTargets) {
      if (target.resolution.has_value()) {
         target.framebuffer.emplace(GAPI_CHECK(target.renderTarget->create_framebuffer(*target.resolution)));
         continue;
      }

      target.framebuffer.emplace(GAPI_CHECK(target.renderTarget->create_framebuffer(resolution)));
   }
}

graphics_api::Framebuffer &NodeFrameResources::framebuffer(const NameID identifier)
{
   auto it = std::ranges::find_if(
           m_renderTargets, [identifier](const RenderTargetResource &res) { return res.name == identifier; });
   assert(it != m_renderTargets.end());
   assert(it->framebuffer.has_value());
   return *it->framebuffer;
}

graphics_api::CommandList &NodeFrameResources::command_list()
{
   assert(m_commandList.has_value());
   return *m_commandList;
}

void NodeFrameResources::add_signal_semaphore(NameID child, graphics_api::Semaphore&& semaphore)
{
   m_signalSemaphores.add_semaphore(semaphore);
   NodeResourcesBase::add_signal_semaphore(child, std::move(semaphore));
}

void NodeFrameResources::initialize_command_list(graphics_api::SemaphoreArray &&waitSemaphores,
                                                 graphics_api::CommandList &&commands)
{
   m_waitSemaphores.emplace(std::move(waitSemaphores));
   m_commandList.emplace(std::move(commands));
}

graphics_api::SemaphoreArray &NodeFrameResources::wait_semaphores()
{
   assert(m_waitSemaphores.has_value());
   return *m_waitSemaphores;
}

graphics_api::SemaphoreArray &NodeFrameResources::signal_semaphores()
{
   return m_signalSemaphores;
}

void FrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   for (auto &node : m_nodes | std::views::values) {
      auto* frameNode = dynamic_cast<NodeFrameResources*>(node.get());
      if (frameNode != nullptr) {
         frameNode->update_resolution(resolution);
      }
   }
}

void FrameResources::add_signal_semaphore(NameID parent, NameID child, graphics_api::Semaphore&& semaphore)
{
   auto &node = m_nodes.at(parent);
   node->add_signal_semaphore(child, std::move(semaphore));
}

void FrameResources::initialize_command_list(NameID nodeName, graphics_api::SemaphoreArray &&waitSemaphores,
                                             graphics_api::CommandList &&commandList)
{
   auto &node = m_nodes.at(nodeName);
   auto* frameNode = dynamic_cast<NodeFrameResources*>(node.get());
   if (frameNode != nullptr) {
      frameNode->initialize_command_list(std::move(waitSemaphores), std::move(commandList));
   }
}

void FrameResources::clean(graphics_api::Device &device)
{
   for (auto &node : m_nodes | std::views::values) {
      node->clean(device);
   }
   m_nodes.clear();
}

bool FrameResources::has_flag(NameID flagName) const
{
   return m_renderFlags.contains(flagName);
}

void FrameResources::set_flag(NameID flagName, bool isEnabled)
{
   if (m_renderFlags.contains(flagName)) {
      if (not isEnabled) {
         m_renderFlags.erase(flagName);
      }
   } else if (isEnabled) {
      m_renderFlags.insert(flagName);
   }
}

void FrameResources::finalize()
{
   for (auto& node : m_nodes | std::views::values) {
      node->finalize();
   }
}

graphics_api::Semaphore& FrameResources::target_semaphore(NameID targetNode)
{
   return this->semaphore(targetNode, "__TARGET__"_name_id);
}

FrameResources::FrameResources(graphics_api::Device &device) :
    m_targetFence(GAPI_CHECK(device.create_fence()))
{
}

graphics_api::Fence &FrameResources::target_fence()
{
   return m_targetFence;
}

void FrameResources::add_external_node(NameID node)
{
   m_nodes.emplace(node, std::make_unique<NodeResourcesBase>());
}

graphics_api::Semaphore& FrameResources::semaphore(NameID parent, NameID child)
{
   return m_nodes[parent]->semaphore(child);
}

}// namespace triglav::render_core
