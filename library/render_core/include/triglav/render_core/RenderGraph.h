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

   void add_external_node(NameID node);
   void add_dependency(NameID target, NameID dependency);
   bool bake(NameID targetNode);
   void initialize_nodes();
   void record_command_lists();
   void set_flag(NameID flag, bool isEnabled);
   void update_resolution(const graphics_api::Resolution &resolution);
   [[nodiscard]] graphics_api::Status execute();
   void await();
   [[nodiscard]] graphics_api::Semaphore& target_semaphore();
   [[nodiscard]] graphics_api::Semaphore& semaphore(NameID parent, NameID child);
   [[nodiscard]] u32 triangle_count(NameID node);
   FrameResources& active_frame_resources();
   void swap_frames();

   void clean();


 private:
   graphics_api::Device &m_device;
   std::set<NameID> m_externalNodes;
   std::map<NameID, std::unique_ptr<IRenderNode>> m_nodes;
   std::multimap<NameID, NameID> m_dependencies;
   std::vector<NameID> m_nodeOrder;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   std::array<FrameResources, 2> m_frameResources;
   NameID m_targetNode{};
   u32 m_activeFrame{0};
};

}// namespace triglav::render_core
