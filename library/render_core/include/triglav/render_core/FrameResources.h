#pragma once

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/RenderTarget.h"
#include "triglav/graphics_api/Synchronization.h"
#include "triglav/Name.hpp"

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <utility>
#include <vector>

namespace triglav::render_core {

class NodeFrameResources
{
 public:
   void add_render_target(NameID identifier, graphics_api::RenderTarget &renderTarget);
   void add_render_target_with_resolution(NameID identifier, graphics_api::RenderTarget &renderTarget, const graphics_api::Resolution &resolution);
   void update_resolution(const graphics_api::Resolution &resolution);

   [[nodiscard]] graphics_api::Framebuffer &framebuffer(NameID identifier);
   [[nodiscard]] graphics_api::CommandList &command_list();
   void add_signal_semaphore(graphics_api::Semaphore *semaphore);
   void initialize_command_list(graphics_api::SemaphoreArray &&waitSemaphores,
                                graphics_api::CommandList &&commands);
   graphics_api::SemaphoreArray &wait_semaphores();
   graphics_api::SemaphoreArray &signal_semaphores();
   void clean(graphics_api::Device &device);

 private:
   struct RenderTargetResource
   {
      NameID name{};
      graphics_api::RenderTarget *renderTarget{};
      std::optional<graphics_api::Framebuffer> framebuffer{};
      std::optional<graphics_api::Resolution> resolution{};
   };

   std::vector<RenderTargetResource> m_renderTargets{};
   NameID m_name{};
   graphics_api::SemaphoreArray m_signalSemaphores;
   std::optional<graphics_api::SemaphoreArray> m_waitSemaphores{};
   std::optional<graphics_api::CommandList> m_commandList{};
   std::vector<graphics_api::Semaphore *> m_signalSemaphorePtrs;
};

class FrameResources
{
 public:
   template<typename TNode>
   auto &add_node_resources(const NameID idenfitier, TNode &&node)
   {
      auto [it, ok] = m_nodes.emplace(idenfitier, std::move(node));
      assert(ok);
      return *it->second;
   }

   template<typename TNode, typename... TArgs>
   TNode &emplace_node_resources(const NameID idenfitier, TArgs &&...args)
   {
      auto [it, ok] = m_nodes.emplace(idenfitier, std::make_unique<TNode>(std::forward<TArgs>(args)...));
      assert(ok);
      return *it->second;
   }

   template<typename TNode = NodeFrameResources>
   TNode &node(const NameID idenfitier)
   {
      return *dynamic_cast<TNode *>(m_nodes.at(idenfitier).get());
   }

   [[nodiscard]] bool has_flag(NameID flagName) const;
   void set_flag(NameID flagName, bool isEnabled);

   void update_resolution(const graphics_api::Resolution &resolution);
   void add_signal_semaphore(NameID nodeName, graphics_api::Semaphore *semaphore);
   void initialize_command_list(NameID nodeName, graphics_api::SemaphoreArray &&waitSemaphores,
                                graphics_api::CommandList &&commandList);
   void clean(graphics_api::Device &device);

 private:
   std::map<NameID, std::unique_ptr<NodeFrameResources>> m_nodes;
   std::set<NameID> m_renderFlags;
};

}// namespace triglav::render_core
