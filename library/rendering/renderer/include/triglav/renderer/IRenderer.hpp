#pragma once

namespace triglav::renderer {

class IRenderer
{
 public:
   virtual ~IRenderer() = default;
   virtual void recreate_render_jobs() = 0;

 private:
};

}// namespace triglav::renderer