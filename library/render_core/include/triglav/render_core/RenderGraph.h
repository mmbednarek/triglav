#pragma once

#include "FrameResources.h"
#include "IRenderNode.hpp"

#include "triglav/graphics_api/QueueManager.h"
#include "triglav/Name.hpp"

#include <map>
#include <memory>
#include <utility>

namespace triglav::render_core {

class FrameResources;

class RenderGraph
{
 public:
   explicit RenderGraph(graphics_api::Device &device);

   template<typename TNode, typename... TArgs>
   void emplace_node(NameID name, TArgs &&...args)
   {
      m_nodes.emplace(name, std::make_unique<TNode>(std::forward<TArgs>(args)...));
   }

   template<typename TNode>
   TNode &node(const NameID name)
   {
      return dynamic_cast<TNode &>(*m_nodes.at(name));
   }

   void add_semaphore_node(NameID node, graphics_api::Semaphore *semaphore);
   void add_dependency(NameID target, NameID dependency);
   bool bake(NameID targetNode);
   void initialize_nodes();
   void record_command_lists();
   void reset_command_lists();
   void set_flag(NameID flag, bool isEnabled);
   void update_resolution(const graphics_api::Resolution &resolution);
   [[nodiscard]] graphics_api::Status execute();
   void await() const;
   [[nodiscard]] graphics_api::Semaphore *target_semaphore() const;
   [[nodiscard]] u32 triangle_count(NameID node);

   void clean();


 private:
   std::map<NameID, graphics_api::Semaphore *> m_semaphoreNodes;
   std::map<NameID, std::unique_ptr<IRenderNode>> m_nodes;
   std::multimap<NameID, NameID> m_dependencies;
   std::vector<NameID> m_nodeOrder;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   FrameResources m_frameResources;
   graphics_api::Fence m_targetFence;
   NameID m_targetNode;
   graphics_api::Device &m_device;
   graphics_api::Semaphore *m_targetSemaphore{};
};

}// namespace triglav::render_core
