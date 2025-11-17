#include "vulkan/Extensions.hpp"

VkResult vkCreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
                                        const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger)
{
   const auto func =
      reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
   if (func == nullptr) {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   }

   return func(instance, p_create_info, p_allocator, p_debug_messenger);
}

void vkDestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debug_messenger,
                                     const VkAllocationCallbacks* p_allocator)
{
   const auto func =
      reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
   if (func == nullptr)
      return;

   func(instance, debug_messenger, p_allocator);
}
