#pragma once

namespace triglav::render_core {

class IRenderer
{
 public:
   virtual ~IRenderer() = default;
   virtual void recreate_render_jobs() = 0;

 private:
};

}// namespace triglav::render_core