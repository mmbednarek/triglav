#pragma once

#include "vulkan/ObjectWrapper.hpp"
#include "Surface.h"
#include "Device.h"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_OBJECT(Instance)

class Instance {
 public:
   explicit Instance(vulkan::Instance&& instance);

   [[nodiscard]] Result<Surface> create_surface(const desktop::ISurface& surface) const;
   [[nodiscard]] Result<DeviceUPtr> create_device(const Surface& surface) const;

   [[nodiscard]] static Result<Instance> create_instance();
 private:
   vulkan::Instance m_instance;
#if GAPI_ENABLE_VALIDATION
   vulkan::DebugUtilsMessengerEXT m_debugMessenger;
#endif
};

}
