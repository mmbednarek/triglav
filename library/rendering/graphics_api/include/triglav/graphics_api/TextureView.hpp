#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(ImageView, Device)

class TextureView
{
 public:
   explicit TextureView(vulkan::ImageView&& image_view, TextureUsageFlags usage_flags);

   [[nodiscard]] VkImageView vulkan_image_view() const;
   [[nodiscard]] TextureUsageFlags usage_flags() const;
   void set_debug_name(std::string_view name) const;

 private:
   vulkan::ImageView m_image_view;
   TextureUsageFlags m_usage_flags;
};

}// namespace triglav::graphics_api
