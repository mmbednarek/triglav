#pragma once

#include "Device.hpp"
#include "Surface.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/Logging.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_OBJECT(Instance)

class Instance
{
   TG_DEFINE_LOG_CATEGORY(VulkanInstance)
 public:
   explicit Instance(vulkan::Instance&& instance);

   [[nodiscard]] Result<Surface> create_surface(const desktop::ISurface& surface) const;
   [[nodiscard]] Result<DeviceUPtr> create_device(const Surface* surface, DevicePickStrategy strategy,
                                                  DeviceFeatureFlags enabled_features) const;
   [[nodiscard]] bool are_features_supported(DeviceFeatureFlags enabled_features) const;

   [[nodiscard]] static Result<Instance> create_instance(const desktop::IDisplay* display);

 private:
   vulkan::Instance m_instance;
#if GAPI_ENABLE_VALIDATION
   vulkan::DebugUtilsMessengerEXT m_debug_messenger;
#endif
};

}// namespace triglav::graphics_api
