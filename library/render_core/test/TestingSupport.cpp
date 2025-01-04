#include "TestingSupport.hpp"

#include <cassert>
#ifdef TG_ENABLE_RENDERDOC
#include <renderdoc_app.h>
#endif// TG_ENABLE_RENDERDOC

namespace triglav::test {

TestingSupport& TestingSupport::the()
{
   static TestingSupport testingSupport;
   return testingSupport;
}

void TestingSupport::initialize_render_doc()
{
#ifdef TG_ENABLE_RENDERDOC
   m_library = std::make_unique<io::DynLibrary>("/usr/lib/librenderdoc.so");
   const auto result = m_library->call<int>("RENDERDOC_GetAPI", eRENDERDOC_API_Version_1_6_0, &m_renderDocApi);
   assert(result == 1);
#endif
}

void TestingSupport::on_quit() const
{
#ifdef TG_ENABLE_RENDERDOC
   if (m_renderDocApi != nullptr) {
      m_renderDocApi->Shutdown();
   }
#endif
}

graphics_api::Device& TestingSupport::device()
{
   return *the().m_device;
}

resource::ResourceManager& TestingSupport::resource_manager()
{
   return *the().m_resourceManager;
}

RENDERDOC_API_1_6_0& TestingSupport::render_doc()
{
   assert(the().m_renderDocApi != nullptr);
   return *the().m_renderDocApi;
}

}// namespace triglav::test