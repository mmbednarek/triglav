#include "FrameResources.h"

#include "triglav/graphics_api/Device.h"

#include <ranges>

namespace triglav::render_core {

using namespace name_literals;

void NodeResourcesBase::add_signal_semaphore(Name child, graphics_api::Semaphore&& semaphore)
{
   m_ownedSemaphores.emplace(child, std::move(semaphore));
}

void NodeResourcesBase::clean(graphics_api::Device &device)
{
   m_ownedSemaphores.clear();
}

graphics_api::Semaphore& NodeResourcesBase::semaphore(Name child)
{
   return m_ownedSemaphores[child];
}

void NodeResourcesBase::finalize()
{
   m_ownedSemaphores.make_heap();
}

void NodeResourcesBase::update_resolution(const graphics_api::Resolution &resolution)
{
}

void NodeFrameResources::add_render_target(const Name identifier, graphics_api::RenderTarget &renderTarget)
{
   m_renderTargets.emplace(identifier, &renderTarget, std::nullopt, std::nullopt);
}

void NodeFrameResources::add_render_target_with_resolution(const Name identifier,
                                                           graphics_api::RenderTarget &renderTarget,
                                                           const graphics_api::Resolution &resolution)
{
   m_renderTargets.emplace(identifier, &renderTarget, std::nullopt, resolution);
}

void NodeFrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   // TODO: Change framebuffer when not used.
   for (auto &target : m_renderTargets) {
      if (not target.has_value())
         continue;

      if (target->second.resolution.has_value()) {
         target->second.framebuffer.emplace(GAPI_CHECK(target->second.renderTarget->create_framebuffer(*target->second.resolution)));
         continue;
      }

      target->second.framebuffer.emplace(GAPI_CHECK(target->second.renderTarget->create_framebuffer(resolution)));
   }
}

graphics_api::Framebuffer &NodeFrameResources::framebuffer(const Name identifier)
{
   auto& fb = m_renderTargets[identifier].framebuffer;
   assert(fb.has_value());
   return *fb;
}

graphics_api::CommandList &NodeFrameResources::command_list()
{
   assert(m_commandList.has_value());
   return *m_commandList;
}

void NodeFrameResources::add_signal_semaphore(Name child, graphics_api::Semaphore&& semaphore)
{
   m_signalSemaphores.add_semaphore(semaphore);
   NodeResourcesBase::add_signal_semaphore(child, std::move(semaphore));
}

void NodeFrameResources::initialize_command_list(graphics_api::SemaphoreArray &&waitSemaphores,
                                                 graphics_api::CommandList &&commands, size_t inFrameWaitSemaphoreCount)
{
   m_waitSemaphores.emplace(std::move(waitSemaphores));
   m_commandList.emplace(std::move(commands));
   m_inFrameWaitSemaphoreCount = inFrameWaitSemaphoreCount;
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

void NodeFrameResources::finalize()
{
   NodeResourcesBase::finalize();
   m_renderTargets.make_heap();
}

size_t NodeFrameResources::in_frame_wait_semaphore_count()
{
   return m_inFrameWaitSemaphoreCount;
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

void FrameResources::add_signal_semaphore(Name parent, Name child, graphics_api::Semaphore&& semaphore)
{
   auto &node = m_nodes.at(parent);
   node->add_signal_semaphore(child, std::move(semaphore));
}

void FrameResources::initialize_command_list(Name nodeName, graphics_api::SemaphoreArray &&waitSemaphores,
                                             graphics_api::CommandList &&commandList, size_t inFrameWaitSemaphoreCount)
{
   auto &node = m_nodes.at(nodeName);
   auto* frameNode = dynamic_cast<NodeFrameResources*>(node.get());
   if (frameNode != nullptr) {
      frameNode->initialize_command_list(std::move(waitSemaphores), std::move(commandList), inFrameWaitSemaphoreCount);
   }
}

void FrameResources::clean(graphics_api::Device &device)
{
   for (auto &node : m_nodes | std::views::values) {
      node->clean(device);
   }
   m_nodes.clear();
}

bool FrameResources::has_flag(Name flagName) const
{
   return m_renderFlags.contains(flagName);
}

void FrameResources::set_flag(Name flagName, bool isEnabled)
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

graphics_api::Semaphore& FrameResources::target_semaphore(Name targetNode)
{
   return this->semaphore(targetNode, "__TARGET__"_name);
}

FrameResources::FrameResources(graphics_api::Device &device) :
    m_targetFence(GAPI_CHECK(device.create_fence()))
{
}

graphics_api::Fence &FrameResources::target_fence()
{
   return m_targetFence;
}

void FrameResources::add_external_node(Name node)
{
   m_nodes.emplace(node, std::make_unique<NodeResourcesBase>());
}

graphics_api::Semaphore& FrameResources::semaphore(Name parent, Name child)
{
   return m_nodes[parent]->semaphore(child);
}

}// namespace triglav::render_core
