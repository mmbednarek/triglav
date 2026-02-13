#include "RenderSupport.hpp"

#include <cassert>
#ifdef TG_ENABLE_RENDERDOC
#include <renderdoc_app.h>
#endif// TG_ENABLE_RENDERDOC

namespace triglav::testing_render_util {

RenderSupport& RenderSupport::the()
{
   static RenderSupport testing_support;
   return testing_support;
}

void RenderSupport::initialize_render_doc()
{
#ifdef TG_ENABLE_RENDERDOC
   m_library = std::make_unique<io::DynLibrary>("/usr/lib/librenderdoc.so");
   const auto result = m_library->call<int>("RENDERDOC_GetAPI", e_renderdoc_API_Version_1_6_0, &m_render_doc_api);
   assert(result == 1);
#endif
}

void RenderSupport::on_quit() const
{
#ifdef TG_ENABLE_RENDERDOC
   if (m_render_doc_api != nullptr) {
      m_render_doc_api->Shutdown();
   }
#endif
}

graphics_api::Device& RenderSupport::device()
{
   return *the().m_device;
}

resource::ResourceManager& RenderSupport::resource_manager()
{
   return *the().m_resource_manager;
}

RENDERDOC_API_1_6_0& RenderSupport::render_doc()
{
   assert(the().m_render_doc_api != nullptr);
   return *the().m_render_doc_api;
}

}// namespace triglav::testing_render_util