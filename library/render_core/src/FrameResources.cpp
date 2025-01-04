#include "FrameResources.hpp"

#include "triglav/NameResolution.hpp"
#include "triglav/graphics_api/Device.hpp"

#include <ranges>
#include <sstream>

namespace triglav::render_core {
using namespace name_literals;

namespace {

[[maybe_unused]]
std::string construct_debug_name(const Name renderTargetName, const Name attachmentName)
{
   std::stringstream ss;
   ss << "RT_" << resolve_name(renderTargetName) << "_" << resolve_name(attachmentName);
   return ss.str();
}

}// namespace

void NodeResourcesBase::add_signal_semaphore(Name child, graphics_api::Semaphore&& semaphore)
{
   m_ownedSemaphores.emplace(child, std::move(semaphore));
}

void NodeResourcesBase::clean(graphics_api::Device& device)
{
   m_ownedSemaphores.clear();
}

graphics_api::Semaphore& NodeResourcesBase::semaphore(const Name child)
{
   return m_ownedSemaphores.at(child);
}

void NodeResourcesBase::update_resolution(const graphics_api::Resolution& resolution) {}

void NodeFrameResources::add_render_target(const Name identifier, graphics_api::RenderTarget& renderTarget)
{
   m_renderTargets.emplace(identifier, RenderTargetResource{&renderTarget, std::nullopt, std::nullopt});
}

void NodeFrameResources::add_render_target_with_resolution(const Name identifier, graphics_api::RenderTarget& renderTarget,
                                                           const graphics_api::Resolution& resolution)
{
   m_renderTargets.emplace(identifier, RenderTargetResource{&renderTarget, std::nullopt, resolution});
}

void NodeFrameResources::add_render_target_with_scale(const Name identifier, graphics_api::RenderTarget& renderTarget,
                                                      const float scaleFactor)
{
   m_renderTargets.emplace(identifier, RenderTargetResource{&renderTarget, std::nullopt, std::nullopt, scaleFactor});
}

void NodeFrameResources::update_resolution(const graphics_api::Resolution& resolution)
{
   // TODO: Change framebuffer when not used.
   for ([[maybe_unused]] auto& [rtName, rtRes] : m_renderTargets) {

      if (rtRes.resolution.has_value()) {
         rtRes.framebuffer.emplace(GAPI_CHECK(rtRes.renderTarget->create_framebuffer(*rtRes.resolution)));
      } else {
         rtRes.framebuffer.emplace(GAPI_CHECK(rtRes.renderTarget->create_framebuffer(resolution * rtRes.scaleFactor)));
      }

      for (const auto& attachment : rtRes.renderTarget->attachments()) {
         auto& attachmentTex = rtRes.framebuffer->texture(attachment.identifier);
         TG_SET_DEBUG_NAME(attachmentTex, construct_debug_name(rtName, attachment.identifier));
         this->register_texture(attachment.identifier, attachmentTex);
      }
   }
}

void NodeFrameResources::register_texture(const Name name, graphics_api::Texture& texture)
{
   m_registeredTextures[name] = &texture;
}

graphics_api::Texture& NodeFrameResources::texture(const Name name) const
{
   assert(m_registeredTextures.contains(name));
   return *m_registeredTextures.at(name);
}

graphics_api::Framebuffer& NodeFrameResources::framebuffer(const Name identifier)
{
   auto& fb = m_renderTargets[identifier].framebuffer;
   assert(fb.has_value());
   return *fb;
}

graphics_api::CommandList& NodeFrameResources::command_list()
{
   assert(m_commandList.has_value());
   return *m_commandList;
}

void NodeFrameResources::add_signal_semaphore(const Name child, graphics_api::Semaphore&& semaphore)
{
   m_signalSemaphores.add_semaphore(semaphore);
   NodeResourcesBase::add_signal_semaphore(child, std::move(semaphore));
}

void NodeFrameResources::initialize_command_list(graphics_api::SemaphoreArray&& waitSemaphores, graphics_api::CommandList&& commands,
                                                 const MemorySize inFrameWaitSemaphoreCount)
{
   m_waitSemaphores.emplace(std::move(waitSemaphores));
   m_commandList.emplace(std::move(commands));
   m_inFrameWaitSemaphoreCount = inFrameWaitSemaphoreCount;
}

graphics_api::SemaphoreArray& NodeFrameResources::wait_semaphores()
{
   assert(m_waitSemaphores.has_value());
   return *m_waitSemaphores;
}

graphics_api::SemaphoreArray& NodeFrameResources::signal_semaphores()
{
   return m_signalSemaphores;
}

MemorySize NodeFrameResources::in_frame_wait_semaphore_count() const
{
   return m_inFrameWaitSemaphoreCount;
}

void FrameResources::update_resolution(const graphics_api::Resolution& resolution)
{
   for (auto& node : m_nodes | std::views::values) {
      auto* frameNode = dynamic_cast<NodeFrameResources*>(node.get());
      if (frameNode != nullptr) {
         frameNode->update_resolution(resolution);
      }
   }
}

void FrameResources::add_signal_semaphore(const Name parent, const Name child, graphics_api::Semaphore&& semaphore) const
{
   auto& node = m_nodes.at(parent);
   node->add_signal_semaphore(child, std::move(semaphore));
}

void FrameResources::initialize_command_list(const Name nodeName, graphics_api::SemaphoreArray&& waitSemaphores,
                                             graphics_api::CommandList&& commandList, const MemorySize inFrameWaitSemaphoreCount) const
{
   auto& node = m_nodes.at(nodeName);
   auto* frameNode = dynamic_cast<NodeFrameResources*>(node.get());
   if (frameNode != nullptr) {
      frameNode->initialize_command_list(std::move(waitSemaphores), std::move(commandList), inFrameWaitSemaphoreCount);
   }
}

void FrameResources::clean(graphics_api::Device& device)
{
   for (auto& node : m_nodes | std::views::values) {
      node->clean(device);
   }
   m_nodes.clear();
}

bool FrameResources::has_flag(const Name flagName) const
{
   return m_renderFlags.contains(flagName);
}

void FrameResources::set_flag(const Name flagName, const bool isEnabled)
{
   if (m_renderFlags.contains(flagName)) {
      if (not isEnabled) {
         m_renderFlags.erase(flagName);
      }
   } else if (isEnabled) {
      m_renderFlags.insert(flagName);
   }
}

u32 FrameResources::get_option_raw(const Name optName) const
{
   const auto opt = m_renderOptions.find(optName);
   if (opt == m_renderOptions.end()) {
      return 0;
   }
   return opt->second;
}

void FrameResources::set_option_raw(const Name optName, const u32 option)
{
   m_renderOptions[optName] = option;
}

graphics_api::Semaphore& FrameResources::target_semaphore(Name targetNode)
{
   return this->semaphore(targetNode, "__TARGET__"_name);
}

FrameResources::FrameResources(graphics_api::Device& device) :
    m_targetFence(GAPI_CHECK(device.create_fence()))
{
}

graphics_api::Fence& FrameResources::target_fence()
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
