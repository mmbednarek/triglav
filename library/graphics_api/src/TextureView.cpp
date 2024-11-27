#include "TextureView.hpp"

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

}// namespace triglav::graphics_api
