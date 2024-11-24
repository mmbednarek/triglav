#include "Instance.hpp"

#include "vulkan/Util.hpp"

#include <spdlog/spdlog.h>

namespace triglav::graphics_api {

namespace {

DeviceFeatureFlags feature_flags_from_physical_device(const VkPhysicalDevice physicalDevice)
{
   VkPhysicalDeviceAccelerationStructureFeaturesKHR asPipelineFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};

   VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
   rtPipelineFeatures.pNext = &asPipelineFeatures;

   VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
   features.pNext = &rtPipelineFeatures;

   vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

   DeviceFeatureFlags result{};

   if (asPipelineFeatures.accelerationStructure && rtPipelineFeatures.rayTracingPipeline) {
      result |= DeviceFeature::RayTracing;
   }

   return result;
}

bool physical_device_pick_predicate(const VkPhysicalDevice physicalDevice, const DevicePickStrategy strategy,
                                    const DeviceFeatureFlags preferredFeatures)
{
   VkPhysicalDeviceProperties props{};
   vkGetPhysicalDeviceProperties(physicalDevice, &props);

   if (strategy != DevicePickStrategy::Any) {
      VkPhysicalDeviceType preferredDeviceType{VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU};
      if (strategy == DevicePickStrategy::PreferIntegrated) {
         preferredDeviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
      }

      if (props.deviceType != preferredDeviceType)
         return false;
   }

   const DeviceFeatureFlags features = feature_flags_from_physical_device(physicalDevice);

   return features & preferredFeatures;
}

auto create_physical_device_pick_predicate(const DevicePickStrategy strategy, const DeviceFeatureFlags preferredFeatures)
{
   return [=](const VkPhysicalDevice physicalDevice) -> bool {
      return physical_device_pick_predicate(physicalDevice, strategy, preferredFeatures);
   };
}

VKAPI_ATTR VkBool32 VKAPI_CALL validation_layers_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
   spdlog::level::level_enum logLevel{spdlog::level::off};

   switch (messageSeverity) {
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      logLevel = spdlog::level::trace;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      logLevel = spdlog::level::info;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      logLevel = spdlog::level::warn;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      logLevel = spdlog::level::err;
      break;
   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
      break;
   }

   spdlog::log(logLevel, "vk-validation: {}", pCallbackData->pMessage);
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
    m_debugMessenger(*m_instance)
#endif
{
#if GAPI_ENABLE_VALIDATION
   VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
   debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debugMessengerInfo.pfnUserCallback = validation_layers_callback;
   debugMessengerInfo.pUserData = nullptr;

   if (const auto res = m_debugMessenger.construct(&debugMessengerInfo); res != VK_SUCCESS) {
      spdlog::error("failed to enable validation layers: {}", static_cast<i32>(res));
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

Result<DeviceUPtr> Instance::create_device(const Surface& surface, const DevicePickStrategy strategy,
                                           const DeviceFeatureFlags enabledFeatures) const
{
   auto physicalDevices = vulkan::get_physical_devices(*m_instance);
   if (physicalDevices.empty()) {
      return std::unexpected{Status::NoSupportedDevicesFound};
   }

   const auto pickPredicate = create_physical_device_pick_predicate(strategy, enabledFeatures);
   auto pickedDevice = std::ranges::find_if(physicalDevices, pickPredicate);
   if (pickedDevice == physicalDevices.end()) {
      const auto pickPredicateAnyWithFeatures = create_physical_device_pick_predicate(DevicePickStrategy::Any, enabledFeatures);
      pickedDevice = std::ranges::find_if(physicalDevices, pickPredicateAnyWithFeatures);
      if (pickedDevice == physicalDevices.end()) {
         return std::unexpected{Status::NoDeviceSupportsRequestedFeatures};
      }
   }

   auto queueFamilies = vulkan::get_queue_family_properties(*pickedDevice);

   std::vector<QueueFamilyInfo> queueFamilyInfos{};
   queueFamilyInfos.reserve(queueFamilies.size());

   u32 queueIndex{};
   for (const auto& family : queueFamilies) {
      VkBool32 canPresent{};
      if (vkGetPhysicalDeviceSurfaceSupportKHR(*pickedDevice, queueIndex, surface.vulkan_surface(), &canPresent) != VK_SUCCESS)
         return std::unexpected(Status::UnsupportedDevice);

      QueueFamilyInfo info{};
      info.index = queueIndex;
      info.queueCount = family.queueCount;
      info.flags = vulkan::vulkan_queue_flags_to_work_type_flags(family.queueFlags, canPresent);

      if (info.flags != WorkType::None) {
         queueFamilyInfos.emplace_back(info);
      }

      ++queueIndex;
   }

   std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
   deviceQueueCreateInfos.resize(queueFamilyInfos.size());

   u32 maxQueues = 0;
   for (const auto& info : queueFamilyInfos) {
      const auto queueCount = info.queueCount;
      if (queueCount > maxQueues) {
         maxQueues = queueCount;
      }
   }

   std::vector<float> queuePriorities{};
   queuePriorities.resize(maxQueues);
   std::ranges::fill(queuePriorities, 1.0f);

   auto deviceQueueCreateInfoIt = deviceQueueCreateInfos.begin();
   for (const auto& info : queueFamilyInfos) {
      deviceQueueCreateInfoIt->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      deviceQueueCreateInfoIt->queueCount = info.queueCount;
      deviceQueueCreateInfoIt->queueFamilyIndex = info.index;
      deviceQueueCreateInfoIt->pQueuePriorities = queuePriorities.data();
      ++deviceQueueCreateInfoIt;
   }

   std::vector vulkanDeviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
      VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
      "VK_KHR_shader_non_semantic_info",
   };

   if (enabledFeatures & DeviceFeature::RayTracing) {
      vulkanDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
      vulkanDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
      vulkanDeviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
      vulkanDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
      vulkanDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
   }

   const auto extensionProperties = vulkan::get_device_extension_properties(*pickedDevice, nullptr);
   for (const auto& property : extensionProperties) {
      const std::string extensionName{property.extensionName};
      if (extensionName == "VK_KHR_portability_subset") {
         vulkanDeviceExtensions.emplace_back("VK_KHR_portability_subset");
         break;
      }
   }

   VkPhysicalDeviceFeatures2 deviceFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
   deviceFeatures.features.sampleRateShading = true;
   deviceFeatures.features.logicOp = true;
   deviceFeatures.features.fillModeNonSolid = true;
   deviceFeatures.features.wideLines = true;
   deviceFeatures.features.samplerAnisotropy = true;
   deviceFeatures.features.shaderInt64 = true;

   VkPhysicalDeviceVulkan12Features vulkan12Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
   vulkan12Features.hostQueryReset = true;
   vulkan12Features.scalarBlockLayout = true;
   vulkan12Features.bufferDeviceAddress = true;
   deviceFeatures.pNext = &vulkan12Features;

   VkPhysicalDeviceVulkan11Features vulkan11Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
   vulkan11Features.variablePointers = true;
   vulkan11Features.variablePointersStorageBuffer = true;
   vulkan11Features.shaderDrawParameters = true;
   vulkan12Features.pNext = &vulkan11Features;

   void** lastFeaturesPtr = &vulkan11Features.pNext;

   VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
   if (enabledFeatures & DeviceFeature::RayTracing) {
      accelerationStructureFeatures.accelerationStructure = true;
      *lastFeaturesPtr = &accelerationStructureFeatures;
      lastFeaturesPtr = &accelerationStructureFeatures.pNext;
   }

   VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
   if (enabledFeatures & DeviceFeature::RayTracing) {
      rayTracingPipelineFeatures.rayTracingPipeline = true;
      *lastFeaturesPtr = &rayTracingPipelineFeatures;
      lastFeaturesPtr = &rayTracingPipelineFeatures.pNext;
   }

   VkDeviceCreateInfo deviceInfo{};
   deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   deviceInfo.pNext = &deviceFeatures;
   deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
   deviceInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
   deviceInfo.enabledExtensionCount = vulkanDeviceExtensions.size();
   deviceInfo.ppEnabledExtensionNames = vulkanDeviceExtensions.data();

   vulkan::Device device;
   if (const auto res = device.construct(*pickedDevice, &deviceInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return std::make_unique<Device>(std::move(device), *pickedDevice, std::move(queueFamilyInfos), enabledFeatures);
}

bool Instance::are_features_supported(const DeviceFeatureFlags enabledFeatures) const
{
   auto physicalDevices = vulkan::get_physical_devices(*m_instance);
   if (physicalDevices.empty()) {
      return false;
   }

   return std::ranges::any_of(physicalDevices, create_physical_device_pick_predicate(DevicePickStrategy::Any, enabledFeatures));
}

#if GAPI_ENABLE_VALIDATION
constexpr std::array g_vulkanInstanceLayers{
   "VK_LAYER_KHRONOS_validation",
};
#else
constexpr std::array<const char*, 0> g_vulkanInstanceLayers{};
#endif

Result<Instance> Instance::create_instance()
{
   VkApplicationInfo appInfo{};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "TRIGLAV Example";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "TRIGLAV Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_3;

   const std::array g_vulkanInstanceExtensions{
      VK_KHR_SURFACE_EXTENSION_NAME,
      desktop::vulkan_extension_name(),
#if GAPI_ENABLE_VALIDATION
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
   };

   VkInstanceCreateInfo instanceInfo{};
   instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   instanceInfo.pApplicationInfo = &appInfo;
   instanceInfo.enabledLayerCount = 0;
   instanceInfo.ppEnabledLayerNames = nullptr;
   instanceInfo.enabledExtensionCount = g_vulkanInstanceExtensions.size();
   instanceInfo.ppEnabledExtensionNames = g_vulkanInstanceExtensions.data();
   instanceInfo.enabledLayerCount = g_vulkanInstanceLayers.size();
   instanceInfo.ppEnabledLayerNames = g_vulkanInstanceLayers.data();

   vulkan::Instance instance;
   if (const auto res = instance.construct(&instanceInfo); res != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return Instance{std::move(instance)};
}

}// namespace triglav::graphics_api
