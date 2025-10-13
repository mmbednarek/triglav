#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(ImageView, Device)

class TextureView
{
 public:
   explicit TextureView(vulkan::ImageView&& imageView, TextureUsageFlags usageFlags);

   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] TextureUsageFlags usage_flags() const;
   void set_debug_name(std::string_view name) const;

 private:
   vulkan::ImageView m_imageView;
   TextureUsageFlags m_usageFlags;
};

}// namespace triglav::graphics_api
