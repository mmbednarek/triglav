#include "Util.h"
#include "GraphicsApi.hpp"


#include <vulkan/vulkan.h>

namespace graphics_api::vulkan {
Result<VkFormat> to_vulkan_color_format(const ColorFormat &format)
{
   switch (format.order) {
   case ColorFormatOrder::RGBA:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16B16A16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8B8A8_UINT;
      case ColorFormatPart::Float32: return VK_FORMAT_R32G32B32A32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::BGRA:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_B8G8R8A8_SRGB;
      case ColorFormatPart::UInt: return VK_FORMAT_B8G8R8A8_UINT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::DS:
      if (format.parts[0] == ColorFormatPart::UNorm16 && format.parts[1] == ColorFormatPart::UInt) {
         return VK_FORMAT_D16_UNORM_S8_UINT;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::RG:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8_SRGB;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8_UINT;
      case ColorFormatPart::Float32: return VK_FORMAT_R32G32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::RGB:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8B8_SRGB;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16B16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8B8_UINT;
      case ColorFormatPart::Float32: return VK_FORMAT_R32G32B32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   }
   return std::unexpected{Status::UnsupportedFormat};
}

Result<ColorFormat> to_color_format(const VkFormat format)
{
   switch (format) {
   case VK_FORMAT_B8G8R8A8_SRGB: return GAPI_COLOR_FORMAT(BGRA, sRGB);
   case VK_FORMAT_R8G8B8A8_SRGB: return GAPI_COLOR_FORMAT(RGBA, sRGB);
   case VK_FORMAT_R32G32B32A32_SFLOAT: return GAPI_COLOR_FORMAT(RGBA, Float32);
   case VK_FORMAT_D16_UNORM_S8_UINT: return GAPI_COLOR_FORMAT(DS, UNorm16, UInt);
   default: break;
   }
   return std::unexpected{Status::UnsupportedFormat};
}

Result<VkColorSpaceKHR> to_vulkan_color_space(ColorSpace colorSpace)
{
   switch (colorSpace) {
   case ColorSpace::sRGB: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
   case ColorSpace::HDR: return VK_COLOR_SPACE_HDR10_ST2084_EXT;
   }

   return std::unexpected{Status::UnsupportedColorSpace};
}

Result<ColorSpace> to_color_space(const VkColorSpaceKHR colorSpace)
{
   switch (colorSpace) {
   case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return ColorSpace::sRGB;
   case VK_COLOR_SPACE_HDR10_ST2084_EXT: return ColorSpace::HDR;
   default: break;
   }

   return std::unexpected{Status::UnsupportedColorSpace};
}

VkShaderStageFlagBits to_vulkan_shader_stage(const ShaderStage stage)
{
   switch (stage) {
   case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
   case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
   }

   return static_cast<VkShaderStageFlagBits>(0);
}

VkShaderStageFlags to_vulkan_shader_stage_flags(ShaderStageFlags flags)
{
   VkShaderStageFlags result{};
   if (flags & ShaderStage::Vertex) {
      result |= VK_SHADER_STAGE_VERTEX_BIT;
   }
   if (flags & ShaderStage::Fragment) {
      result |= VK_SHADER_STAGE_FRAGMENT_BIT;
   }
   return result;
}

VkDescriptorType to_vulkan_descriptor_type(DescriptorType descriptorType)
{
   switch (descriptorType) {
   case DescriptorType::UniformBuffer:
      return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   case DescriptorType::Sampler:
      return VK_DESCRIPTOR_TYPE_SAMPLER;
   case DescriptorType::ImageSampler:
      return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   }
   return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

}// namespace graphics_api::vulkan
