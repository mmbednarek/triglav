#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "GraphicsApi.hpp"

namespace graphics_api {

struct Subpass
{
   std::vector<VkAttachmentReference> references{};
   VkSubpassDescription description{};
};

class IRenderTarget
{
 public:
   virtual ~IRenderTarget() = default;

   [[nodiscard]] virtual Subpass vulkan_subpass()                                       = 0;
   [[nodiscard]] virtual std::vector<VkAttachmentDescription> vulkan_attachments()      = 0;
   [[nodiscard]] virtual std::vector<VkSubpassDependency> vulkan_subpass_dependencies() = 0;
   [[nodiscard]] virtual Resolution resolution() const                                  = 0;
   [[nodiscard]] virtual SampleCount sample_count() const                               = 0;
   [[nodiscard]] virtual int color_attachment_count() const                             = 0;
};

}// namespace graphics_api