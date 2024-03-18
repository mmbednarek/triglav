#pragma once

#include "triglav/graphics_api/CommandList.h"

namespace triglav::render_core {

class IRenderNode
{
 public:
   virtual ~IRenderNode() = default;

   [[nodiscard]] virtual graphics_api::WorkTypeFlags work_types() const = 0;
   virtual void record_commands(graphics_api::CommandList &cmdList) = 0;
};

}// namespace triglav::render_core