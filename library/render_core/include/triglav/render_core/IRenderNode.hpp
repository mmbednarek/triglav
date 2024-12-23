#pragma once

#include "FrameResources.hpp"

#include "triglav/graphics_api/CommandList.hpp"

#include <memory>

namespace triglav::render_core {

class IRenderNode
{
 public:
   virtual ~IRenderNode() = default;

   [[nodiscard]] virtual graphics_api::WorkTypeFlags work_types() const = 0;
   virtual void record_commands(FrameResources& frameResources, NodeFrameResources& nodeResources, graphics_api::CommandList& cmdList) = 0;

   virtual std::unique_ptr<NodeFrameResources> create_node_resources()
   {
      return std::make_unique<NodeFrameResources>();
   }
};

}// namespace triglav::render_core