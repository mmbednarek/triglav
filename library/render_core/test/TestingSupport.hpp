#pragma once

namespace triglav::graphics_api {
class Device;
}

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::test {

class TestingSupport
{
 public:
   static TestingSupport& the();

   graphics_api::Device* m_device;
   resource::ResourceManager* m_resourceManager;

   static graphics_api::Device& device()
   {
      return *the().m_device;
   }

   static resource::ResourceManager& resource_manager()
   {
      return *the().m_resourceManager;
   }
};

}// namespace triglav::test