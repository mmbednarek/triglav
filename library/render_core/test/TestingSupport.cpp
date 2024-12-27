#include "TestingSupport.hpp"

#include <cassert>
#include <renderdoc_app.h>

namespace triglav::test {

TestingSupport& TestingSupport::the()
{
   static TestingSupport testingSupport;
   return testingSupport;
}

void TestingSupport::initialize_render_doc()
{
   m_library = std::make_unique<io::DynLibrary>("/usr/lib/librenderdoc.so");
   const auto result = m_library->call<int>("RENDERDOC_GetAPI", eRENDERDOC_API_Version_1_6_0, &m_renderDocApi);
   assert(result == 1);
}

void TestingSupport::on_quit() const
{
   if (m_renderDocApi != nullptr) {
      m_renderDocApi->Shutdown();
   }
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
   return *the().m_renderDocApi;
}

}// namespace triglav::test