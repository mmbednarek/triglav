#pragma once

#include "triglav/io/DynLibrary.hpp"

#include <memory>

struct RENDERDOC_API_1_6_0;

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
   resource::ResourceManager* m_resource_manager;
   std::unique_ptr<io::DynLibrary> m_library;
   RENDERDOC_API_1_6_0* m_render_doc_api;

   void initialize_render_doc();
   void on_quit() const;

   static graphics_api::Device& device();
   static resource::ResourceManager& resource_manager();
   static RENDERDOC_API_1_6_0& render_doc();
};

}// namespace triglav::test