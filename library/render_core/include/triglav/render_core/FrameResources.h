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

class NodeResourcesBase
{
 public:
   virtual ~NodeResourcesBase() = default;

   virtual void add_signal_semaphore(NameID child, graphics_api::Semaphore&& semaphore);
   virtual void clean(graphics_api::Device &device);
   virtual void finalize();
   [[nodiscard]] virtual graphics_api::Semaphore& semaphore(NameID child);

 private:
   Heap<NameID, graphics_api::Semaphore> m_ownedSemaphores{};
};

class NodeFrameResources : public NodeResourcesBase
{
 public:
   void add_render_target(NameID identifier, graphics_api::RenderTarget &renderTarget);
   void add_render_target_with_resolution(NameID identifier, graphics_api::RenderTarget &renderTarget, const graphics_api::Resolution &resolution);
   void update_resolution(const graphics_api::Resolution &resolution);

   [[nodiscard]] graphics_api::Framebuffer &framebuffer(NameID identifier);
   [[nodiscard]] graphics_api::CommandList &command_list();
   void add_signal_semaphore(NameID child, graphics_api::Semaphore&& semaphore) override;
   void initialize_command_list(graphics_api::SemaphoreArray &&waitSemaphores,
                                graphics_api::CommandList &&commands);
   graphics_api::SemaphoreArray &wait_semaphores();
   graphics_api::SemaphoreArray &signal_semaphores();

 private:
   struct RenderTargetResource
   {
      NameID name{};
      graphics_api::RenderTarget *renderTarget{};
      std::optional<graphics_api::Framebuffer> framebuffer{};
      std::optional<graphics_api::Resolution> resolution{};
   };

   std::vector<RenderTargetResource> m_renderTargets{};
   graphics_api::SemaphoreArray m_signalSemaphores;
   std::optional<graphics_api::SemaphoreArray> m_waitSemaphores{};
   std::optional<graphics_api::CommandList> m_commandList{};
};

class FrameResources
{
 public:
   explicit FrameResources(graphics_api::Device& device);

   template<typename TNode>
   auto &add_node_resources(const NameID identifier, TNode &&node)
   {
      auto [it, ok] = m_nodes.emplace(identifier, std::forward<TNode>(node));
      assert(ok);
      return *it->second;
   }

   template<typename TNode = NodeFrameResources>
   TNode &node(const NameID identifier)
   {
      return *dynamic_cast<TNode *>(m_nodes.at(identifier).get());
   }

   void add_external_node(NameID node);

   [[nodiscard]] graphics_api::Semaphore& target_semaphore(NameID targetNode);
   [[nodiscard]] graphics_api::Fence& target_fence();
   [[nodiscard]] graphics_api::Semaphore& semaphore(NameID parent, NameID child);

   [[nodiscard]] bool has_flag(NameID flagName) const;
   void set_flag(NameID flagName, bool isEnabled);

   void update_resolution(const graphics_api::Resolution &resolution);
   void add_signal_semaphore(NameID parent, NameID child, graphics_api::Semaphore&& semaphore);
   void initialize_command_list(NameID nodeName, graphics_api::SemaphoreArray &&waitSemaphores,
                                graphics_api::CommandList &&commandList);
   void clean(graphics_api::Device &device);
   void finalize();

 private:
   std::map<NameID, std::unique_ptr<NodeResourcesBase>> m_nodes;
   std::set<NameID> m_renderFlags;
   graphics_api::Fence m_targetFence;
};

}// namespace triglav::render_core
