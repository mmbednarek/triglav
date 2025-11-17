#include "Instance.hpp"

#include "vulkan/Util.hpp"

#include "triglav/String.hpp"

#define TG_ENABLE_SYNC_VALIDATION 0

namespace triglav::graphics_api {

namespace {

DeviceFeatureFlags feature_flags_from_physical_device(const VkPhysicalDevice physical_device)
{
   VkPhysicalDeviceAccelerationStructureFeaturesKHR as_pipeline_features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};

   VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
   rt_pipeline_features.pNext = &as_pipeline_features;

   VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
   features.pNext = &rt_pipeline_features;

   vkGetPhysicalDeviceFeatures2(physical_device, &features);

   DeviceFeatureFlags result{};

   if (as_pipeline_features.accelerationStructure && rt_pipeline_features.rayTracingPipeline) {
      result |= DeviceFeature::RayTracing;
   }

   return result;
}

bool physical_device_pick_predicate(const VkPhysicalDevice physical_device, const DevicePickStrategy strategy,
                                    const DeviceFeatureFlags preferred_features)
{
   VkPhysicalDeviceProperties props{};
   vkGetPhysicalDeviceProperties(physical_device, &props);

   if (strategy != DevicePickStrategy::Any) {
      VkPhysicalDeviceType preferred_device_type{VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU};
      if (strategy == DevicePickStrategy::PreferIntegrated) {
         preferred_device_type = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
      }

      if (props.deviceType != preferred_device_type)
         return false;
   }

   const DeviceFeatureFlags features = feature_flags_from_physical_device(physical_device);

   return features & preferred_features;
}

auto create_physical_device_pick_predicate(const DevicePickStrategy strategy, const DeviceFeatureFlags preferred_features)
{
   return [=](const VkPhysicalDevice physical_device) -> bool {
      return physical_device_pick_predicate(physical_device, strategy, preferred_features);
   };
}

[[maybe_unused]]
VKAPI_ATTR VkBool32 VKAPI_CALL validation_layers_callback(const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                          VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
                                                          const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                          void* /*p_user_data*/)
{
   using namespace string_literals;

   LogLevel log_level{};

   switch (message_severity) {
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      log_level = LogLevel::Debug;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      log_level = LogLevel::Info;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log_level = LogLevel::Warning;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      log_level = LogLevel::Error;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
      break;
   }

   log_message(log_level, "VulkanValidation"_strv, "{}", p_callback_data->pMessage);
   return VK_FALSE;
}

}// namespace

DECLARE_VLK_ENUMERATOR(get_physical_devices, VkPhysicalDevice, vkEnumeratePhysicalDevices)
DECLARE_VLK_ENUMERATOR(get_queue_family_properties, VkQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties)
DECLARE_VLK_ENUMERATOR(get_device_extension_properties, VkExtensionProperties, vkEnumerateDeviceExtensionProperties)

Instance::Instance(vulkan::Instance&& instance) :
    m_instance(std::move(instance))
#if GAPI_ENABLE_VALIDATION
    ,
    m_debug_messenger(*m_instance)
#endif
{
#if GAPI_ENABLE_VALIDATION
   VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
   debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debug_messenger_info.pfnUserCallback = validation_layers_callback;
   debug_messenger_info.pUserData = nullptr;

   if (const auto res = m_debug_messenger.construct(&debug_messenger_info); res != VK_SUCCESS) {
      log_error("failed to enable validation layers: {}", static_cast<i32>(res));
   }

#endif
}

Result<Surface> Instance::create_surface(const desktop::ISurface& surface) const
{
   vulkan::SurfaceKHR vulkan_surface(*m_instance);
   if (const auto res = vulkan_surface.construct(&surface); res != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return Surface{std::move(vulkan_surface)};
}

Result<DeviceUPtr> Instance::create_device(const Surface* surface, const DevicePickStrategy strategy,
                                           const DeviceFeatureFlags enabled_features) const
{
   auto physical_devices = vulkan::get_physical_devices(*m_instance);
   if (physical_devices.empty()) {
      return std::unexpected{Status::NoSupportedDevicesFound};
   }

   const auto pick_predicate = create_physical_device_pick_predicate(strategy, enabled_features);
   auto picked_device = std::ranges::find_if(physical_devices, pick_predicate);
   if (picked_device == physical_devices.end()) {
      const auto pick_predicate_any_with_features = create_physical_device_pick_predicate(DevicePickStrategy::Any, enabled_features);
      picked_device = std::ranges::find_if(physical_devices, pick_predicate_any_with_features);
      if (picked_device == physical_devices.end()) {
         return std::unexpected{Status::NoDeviceSupportsRequestedFeatures};
      }
   }

   auto queue_families = vulkan::get_queue_family_properties(*picked_device);

   std::vector<QueueFamilyInfo> queue_family_infos{};
   queue_family_infos.reserve(queue_families.size());

   u32 queue_index{};
   for (const auto& family : queue_families) {
      VkBool32 can_present{false};
      if (surface != nullptr) {
         if (vkGetPhysicalDeviceSurfaceSupportKHR(*picked_device, queue_index, surface->vulkan_surface(), &can_present) != VK_SUCCESS)
            return std::unexpected(Status::UnsupportedDevice);
      }

      QueueFamilyInfo info{};
      info.index = queue_index;
      info.queue_count = family.queueCount;
      info.flags = vulkan::vulkan_queue_flags_to_work_type_flags(family.queueFlags, can_present);

      if (info.flags != WorkType::None) {
         queue_family_infos.emplace_back(info);
      }

      ++queue_index;
   }

   std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;
   device_queue_create_infos.resize(queue_family_infos.size());

   u32 max_queues = 0;
   for (const auto& info : queue_family_infos) {
      const auto queue_count = info.queue_count;
      if (queue_count > max_queues) {
         max_queues = queue_count;
      }
   }

   std::vector<float> queue_priorities{};
   queue_priorities.resize(max_queues);
   std::ranges::fill(queue_priorities, 1.0f);

   auto device_queue_create_info_it = device_queue_create_infos.begin();
   for (const auto& info : queue_family_infos) {
      device_queue_create_info_it->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      device_queue_create_info_it->queueCount = info.queue_count;
      device_queue_create_info_it->queueFamilyIndex = info.index;
      device_queue_create_info_it->pQueuePriorities = queue_priorities.data();
      ++device_queue_create_info_it;
   }

   std::vector vulkan_device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
      VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
      "VK_KHR_shader_non_semantic_info",
   };

   if (enabled_features & DeviceFeature::RayTracing) {
      vulkan_device_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
      vulkan_device_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
      vulkan_device_extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
      vulkan_device_extensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
      vulkan_device_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
   }

   const auto extension_properties = vulkan::get_device_extension_properties(*picked_device, nullptr);
   for (const auto& property : extension_properties) {
      const std::string extension_name{property.extensionName};
      if (extension_name == "VK_KHR_portability_subset") {
         vulkan_device_extensions.emplace_back("VK_KHR_portability_subset");
         break;
      }
   }

   VkPhysicalDeviceFeatures2 device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
   device_features.features.sampleRateShading = true;
   device_features.features.logicOp = true;
   device_features.features.fillModeNonSolid = true;
   device_features.features.wideLines = true;
   device_features.features.samplerAnisotropy = true;
   device_features.features.shaderInt64 = true;
   device_features.features.pipelineStatisticsQuery = true;
   device_features.features.textureCompressionBC = true;

   VkPhysicalDeviceVulkan13Features vulkan13_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
   vulkan13_features.dynamicRendering = true;
   device_features.pNext = &vulkan13_features;

   VkPhysicalDeviceVulkan12Features vulkan12_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
   vulkan12_features.hostQueryReset = true;
   vulkan12_features.scalarBlockLayout = true;
   vulkan12_features.bufferDeviceAddress = true;
   vulkan12_features.drawIndirectCount = true;
   vulkan12_features.runtimeDescriptorArray = true;
   vulkan13_features.pNext = &vulkan12_features;

   VkPhysicalDeviceVulkan11Features vulkan11_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
   vulkan11_features.variablePointers = true;
   vulkan11_features.variablePointersStorageBuffer = true;
   vulkan11_features.shaderDrawParameters = true;
   vulkan12_features.pNext = &vulkan11_features;

   void** last_features_ptr = &vulkan11_features.pNext;

   VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
   if (enabled_features & DeviceFeature::RayTracing) {
      acceleration_structure_features.accelerationStructure = true;
      *last_features_ptr = &acceleration_structure_features;
      last_features_ptr = &acceleration_structure_features.pNext;
   }

   VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
   if (enabled_features & DeviceFeature::RayTracing) {
      ray_tracing_pipeline_features.rayTracingPipeline = true;
      *last_features_ptr = &ray_tracing_pipeline_features;
      last_features_ptr = &ray_tracing_pipeline_features.pNext;
   }

   VkDeviceCreateInfo device_info{};
   device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   device_info.pNext = &device_features;
   device_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());
   device_info.pQueueCreateInfos = device_queue_create_infos.data();
   device_info.enabledExtensionCount = static_cast<u32>(vulkan_device_extensions.size());
   device_info.ppEnabledExtensionNames = vulkan_device_extensions.data();

   vulkan::Device device;
   if (const auto res = device.construct(*picked_device, &device_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return std::make_unique<Device>(std::move(device), *picked_device, std::move(queue_family_infos), enabled_features);
}

bool Instance::are_features_supported(const DeviceFeatureFlags enabled_features) const
{
   auto physical_devices = vulkan::get_physical_devices(*m_instance);
   if (physical_devices.empty()) {
      return false;
   }

   return std::ranges::any_of(physical_devices, create_physical_device_pick_predicate(DevicePickStrategy::Any, enabled_features));
}

#if GAPI_ENABLE_VALIDATION
constexpr std::array g_vulkan_instance_layers{
   "VK_LAYER_KHRONOS_validation",
};
#else
constexpr std::array<const char*, 0> g_vulkan_instance_layers{};
#endif

Result<Instance> Instance::create_instance(const desktop::IDisplay* display)
{
   VkApplicationInfo app_info{};
   app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   app_info.pApplicationName = "TRIGLAV Example";
   app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   app_info.pEngineName = "TRIGLAV Engine";
   app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   app_info.apiVersion = VK_API_VERSION_1_3;

   std::vector vulkan_instance_extensions{
      VK_KHR_SURFACE_EXTENSION_NAME,
#if GAPI_ENABLE_VALIDATION
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
   };

   if (display != nullptr) {
      vulkan_instance_extensions.push_back(desktop::vulkan_extension_name(display));
   }

   VkInstanceCreateInfo instance_info{};
   instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   instance_info.pApplicationInfo = &app_info;
   instance_info.enabledExtensionCount = static_cast<u32>(vulkan_instance_extensions.size());
   instance_info.ppEnabledExtensionNames = vulkan_instance_extensions.data();
   instance_info.enabledLayerCount = static_cast<u32>(g_vulkan_instance_layers.size());
   instance_info.ppEnabledLayerNames = g_vulkan_instance_layers.data();

#if TG_ENABLE_SYNC_VALIDATION
   VkValidationFeaturesEXT validation_features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
   constexpr std::array<VkValidationFeatureEnableEXT, 1> enabled_features{VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};
   validation_features.enabled_validation_feature_count = enabled_features.size();
   validation_features.p_enabled_validation_features = enabled_features.data();
   instance_info.pNext = &validation_features;
#endif

   vulkan::Instance instance;
   if (const auto res = instance.construct(&instance_info); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Instance{std::move(instance)};
}

}// namespace triglav::graphics_api
