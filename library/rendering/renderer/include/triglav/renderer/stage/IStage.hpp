#pragma once

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {
struct Config;
}

namespace triglav::renderer::stage {

class IStage
{
 public:
   virtual ~IStage() = default;

   virtual void build_stage(render_core::BuildContext& ctx, const Config& config) const = 0;
};

}// namespace triglav::renderer::stage
