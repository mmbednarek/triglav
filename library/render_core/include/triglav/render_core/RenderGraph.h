#pragma once

#include "IRenderNode.hpp"

#include "triglav/graphics_api/QueueManager.h"
#include "triglav/Name.hpp"

#include <map>
#include <memory>
#include <utility>

namespace triglav::render_core {

class RenderGraph
{
 public:
   struct BakedNode
   {
      NameID name;
      graphics_api::SemaphoreArray waitSemaphores;
      graphics_api::Semaphore *singalSemaphore;
      graphics_api::CommandList commandList;
   };

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
   void record_command_lists();
   [[nodiscard]] graphics_api::Status execute();
   void await() const;
   graphics_api::Semaphore *target_semaphore() const;

   void clean();


 private:
   std::map<NameID, graphics_api::Semaphore *> m_semaphoreNodes;
   std::map<NameID, std::unique_ptr<IRenderNode>> m_nodes;
   std::multimap<NameID, NameID> m_dependencies;
   std::vector<BakedNode> m_bakedNodes;
   std::map<NameID, u32> m_nodeIndicies;
   graphics_api::Fence m_targetFence;
   graphics_api::Device &m_device;
};

}// namespace triglav::render_core
