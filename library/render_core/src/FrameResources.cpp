#include "FrameResources.h"

#include "triglav/graphics_api/Device.h"

#include <ranges>

namespace triglav::render_core {

void NodeFrameResources::add_render_target(const NameID identifier, graphics_api::RenderTarget &renderTarget)
{
   m_renderTargets.emplace_back(identifier, &renderTarget, std::nullopt);
}

void NodeFrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   for (auto &target : m_renderTargets) {
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

void NodeFrameResources::add_signal_semaphore(graphics_api::Semaphore *semaphore)
{
   m_signalSemaphorePtrs.emplace_back(semaphore);
   m_signalSemaphores.add_semaphore(*semaphore);
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

void NodeFrameResources::clean(graphics_api::Device &device)
{
   for (const auto *semaphorePtr : m_signalSemaphorePtrs) {
      device.queue_manager().release_semaphore(semaphorePtr);
   }
}

void FrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   for (auto &node : std::views::values(m_nodes)) {
      node->update_resolution(resolution);
   }
}

void FrameResources::add_signal_semaphore(NameID nodeName, graphics_api::Semaphore *semaphore)
{
   auto &node = m_nodes.at(nodeName);
   node->add_signal_semaphore(semaphore);
}

void FrameResources::initialize_command_list(NameID nodeName, graphics_api::SemaphoreArray &&waitSemaphores,
                                             graphics_api::CommandList &&commandList)
{
   auto &node = m_nodes.at(nodeName);
   node->initialize_command_list(std::move(waitSemaphores), std::move(commandList));
}

void FrameResources::clean(graphics_api::Device &device)
{
   for (auto &node : m_nodes | std::views::values) {
      node->clean(device);
   }
   m_nodes.clear();
}

}// namespace triglav::render_core
