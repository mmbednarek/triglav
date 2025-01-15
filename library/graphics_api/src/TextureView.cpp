#include "TextureView.hpp"

#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api {

TextureView::TextureView(vulkan::ImageView&& imageView, const TextureUsageFlags usageFlags) :
    m_imageView(std::move(imageView)),
    m_usageFlags(usageFlags)
{
}

VkImageView TextureView::vulkan_image_view() const
{
   return *m_imageView;
}

TextureUsageFlags TextureView::usage_flags() const
{
   return m_usageFlags;
}

void TextureView::set_debug_name(const std::string_view name) const
{
   if (name.empty())
      return;

   VkDebugUtilsObjectNameInfoEXT debugUtilsObjectName{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debugUtilsObjectName.objectHandle = reinterpret_cast<u64>(*m_imageView);
   debugUtilsObjectName.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
   debugUtilsObjectName.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_imageView.parent(), &debugUtilsObjectName);
   assert(result == VK_SUCCESS);
}

}// namespace triglav::graphics_api
