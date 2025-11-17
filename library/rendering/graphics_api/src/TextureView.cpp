#include "TextureView.hpp"

#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api {

TextureView::TextureView(vulkan::ImageView&& image_view, const TextureUsageFlags usage_flags) :
    m_image_view(std::move(image_view)),
    m_usage_flags(usage_flags)
{
}

VkImageView TextureView::vulkan_image_view() const
{
   return *m_image_view;
}

TextureUsageFlags TextureView::usage_flags() const
{
   return m_usage_flags;
}

void TextureView::set_debug_name(const std::string_view name) const
{
   if (name.empty())
      return;

   VkDebugUtilsObjectNameInfoEXT debug_utils_object_name{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(*m_image_view);
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
   debug_utils_object_name.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_image_view.parent(), &debug_utils_object_name);
   assert(result == VK_SUCCESS);
}

}// namespace triglav::graphics_api
