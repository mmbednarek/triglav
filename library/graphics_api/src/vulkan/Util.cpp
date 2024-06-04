#include "Util.h"
#include "GraphicsApi.hpp"


#include <vulkan/vulkan.h>

namespace triglav::graphics_api::vulkan {
Result<VkFormat> to_vulkan_color_format(const ColorFormat &format)
{
   switch (format.order) {
   case ColorFormatOrder::R:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8_SRGB;
      case ColorFormatPart::UNorm8: return VK_FORMAT_R8_UNORM;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8_UINT;
      case ColorFormatPart::Float16: return VK_FORMAT_R16_SFLOAT;
      case ColorFormatPart::Float32: return VK_FORMAT_R32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::RGBA:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
      case ColorFormatPart::UNorm8: return VK_FORMAT_R8G8B8A8_UNORM;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16B16A16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8B8A8_UINT;
      case ColorFormatPart::Float16: return VK_FORMAT_R16G16B16A16_SFLOAT;
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
   case ColorFormatOrder::D:
      switch (format.parts[0]) {
      case ColorFormatPart::UNorm16: return VK_FORMAT_D16_UNORM;
      case ColorFormatPart::Float32: return VK_FORMAT_D32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::RG:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8_SRGB;
      case ColorFormatPart::UNorm8: return VK_FORMAT_R8G8_UNORM;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8_UINT;
      case ColorFormatPart::Float16: return VK_FORMAT_R16G16_SFLOAT;
      case ColorFormatPart::Float32: return VK_FORMAT_R32G32_SFLOAT;
      default: break;
      }
      return std::unexpected{Status::UnsupportedFormat};
   case ColorFormatOrder::RGB:
      switch (format.parts[0]) {
      case ColorFormatPart::sRGB: return VK_FORMAT_R8G8B8_SRGB;
      case ColorFormatPart::UNorm8: return VK_FORMAT_R8G8B8_UNORM;
      case ColorFormatPart::UNorm16: return VK_FORMAT_R16G16B16_UNORM;
      case ColorFormatPart::UInt: return VK_FORMAT_R8G8B8_UINT;
      case ColorFormatPart::Float16: return VK_FORMAT_R16G16B16_SFLOAT;
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
   case VK_FORMAT_B8G8R8A8_SRGB: return GAPI_FORMAT(BGRA, sRGB);
   case VK_FORMAT_R8G8B8A8_SRGB: return GAPI_FORMAT(RGBA, sRGB);
   case VK_FORMAT_R32G32B32A32_SFLOAT: return GAPI_FORMAT(RGBA, Float32);
   case VK_FORMAT_D16_UNORM_S8_UINT: return GAPI_FORMAT(DS, UNorm16, UInt);
   default: break;
   }
   return std::unexpected(Status::UnsupportedFormat);
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

WorkTypeFlags vulkan_queue_flags_to_work_type_flags(const VkQueueFlags flags, const bool canPresent)
{
   WorkTypeFlags result{WorkType::None};
   if (flags & VK_QUEUE_GRAPHICS_BIT) {
      result |= WorkType::Graphics;
   }
   if (flags & VK_QUEUE_TRANSFER_BIT) {
      result |= WorkType::Transfer;
   }
   if (flags & VK_QUEUE_COMPUTE_BIT) {
      result |= WorkType::Compute;
   }
   if (canPresent) {
      result |= WorkType::Presentation;
   }

   return result;
}

Result<VkSampleCountFlagBits> to_vulkan_sample_count(const SampleCount sampleCount)
{
   switch (sampleCount) {
   case SampleCount::Single: return VK_SAMPLE_COUNT_1_BIT;
   case SampleCount::Double: return VK_SAMPLE_COUNT_2_BIT;
   case SampleCount::Quadruple: return VK_SAMPLE_COUNT_4_BIT;
   case SampleCount::Octuple: return VK_SAMPLE_COUNT_8_BIT;
   case SampleCount::Sexdecuple: return VK_SAMPLE_COUNT_16_BIT;
   }

   return std::unexpected{Status::UnsupportedFormat};
}

Result<VkImageLayout> to_vulkan_image_layout(const AttachmentAttributeFlags type)
{
   if (type & AttachmentAttribute::Presentable) {
      return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   }
   if (type & AttachmentAttribute::Depth) {
      if (type & AttachmentAttribute::StoreImage)
         return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
   }
   if (type & AttachmentAttribute::Color) {
      return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   }

   return std::unexpected{Status::UnsupportedFormat};
}

std::tuple<VkAttachmentLoadOp, VkAttachmentStoreOp>
to_vulkan_load_store_ops(const AttachmentAttributeFlags flags)
{
   VkAttachmentLoadOp loadOp{};
   if (flags & AttachmentAttribute::ClearImage)
      loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   else if (flags & AttachmentAttribute::LoadImage)
      loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
   else
      loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

   VkAttachmentStoreOp storeOp;
   if (flags & AttachmentAttribute::StoreImage)
      storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   else
      storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

   return {loadOp, storeOp};
}

VkShaderStageFlagBits to_vulkan_shader_stage(const PipelineStage stage)
{
   switch (stage) {
   case PipelineStage::VertexShader: return VK_SHADER_STAGE_VERTEX_BIT;
   case PipelineStage::FragmentShader: return VK_SHADER_STAGE_FRAGMENT_BIT;
   case PipelineStage::ComputeShader: return VK_SHADER_STAGE_COMPUTE_BIT;
   }

   return static_cast<VkShaderStageFlagBits>(0);
}

VkShaderStageFlags to_vulkan_shader_stage_flags(PipelineStageFlags flags)
{
   VkShaderStageFlags result{};
   if (flags & PipelineStage::VertexShader) {
      result |= VK_SHADER_STAGE_VERTEX_BIT;
   }
   if (flags & PipelineStage::FragmentShader) {
      result |= VK_SHADER_STAGE_FRAGMENT_BIT;
   }
   if (flags & PipelineStage::ComputeShader) {
      result |= VK_SHADER_STAGE_COMPUTE_BIT;
   }
   return result;
}

VkPipelineStageFlagBits to_vulkan_pipeline_stage(const PipelineStage stage)
{
   switch (stage) {
   case PipelineStage::Entrypoint: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
   case PipelineStage::VertexShader: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
   case PipelineStage::FragmentShader: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   case PipelineStage::ComputeShader: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
   case PipelineStage::Transfer: return VK_PIPELINE_STAGE_TRANSFER_BIT;
   case PipelineStage::End: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
   default: break;
   }
   return VK_PIPELINE_STAGE_NONE;
}

VkPipelineStageFlags to_vulkan_pipeline_stage_flags(const PipelineStageFlags flags)
{
   VkPipelineStageFlags result{};
   if (flags & PipelineStage::Entrypoint) {
      result |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
   }
   if (flags & PipelineStage::VertexShader) {
      result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
   }
   if (flags & PipelineStage::FragmentShader) {
      result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
   }
   if (flags & PipelineStage::ComputeShader) {
      result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
   }
   if (flags & PipelineStage::Transfer) {
      result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
   }
   return result;
}

VkDescriptorType to_vulkan_descriptor_type(DescriptorType descriptorType)
{
   switch (descriptorType) {
   case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
   case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
   case DescriptorType::ImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   }
   return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkImageLayout to_vulkan_image_layout(const TextureState resourceState)
{
   switch (resourceState) {
   case TextureState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
   case TextureState::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
   case TextureState::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   case TextureState::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   }

   return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags to_vulkan_access_flags(const TextureState resourceState)
{
   switch (resourceState) {
   case TextureState::Undefined: return 0;
   case TextureState::TransferSrc: return VK_ACCESS_TRANSFER_READ_BIT;
   case TextureState::TransferDst: return VK_ACCESS_TRANSFER_WRITE_BIT;
   case TextureState::ShaderRead: return VK_ACCESS_SHADER_READ_BIT;
   }

   return 0;
}

VkFilter to_vulkan_filter(const FilterType filterType)
{
   switch (filterType) {
   case FilterType::Linear: return VK_FILTER_LINEAR;
   case FilterType::NearestNeighbour: return VK_FILTER_NEAREST;
   }
   return VK_FILTER_MAX_ENUM;
}

VkSamplerAddressMode to_vulkan_sampler_address_mode(const TextureAddressMode mode)
{
   switch (mode) {
   case TextureAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
   case TextureAddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
   case TextureAddressMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
   case TextureAddressMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
   case TextureAddressMode::MirrorOnce: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
   }

   return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

VkImageUsageFlags to_vulkan_image_usage_flags(const TextureUsageFlags usage)
{
   VkImageUsageFlags result{};
   if (usage & TextureUsage::Sampled) {
      result |= VK_IMAGE_USAGE_SAMPLED_BIT;
   }
   if (usage & TextureUsage::TransferSrc) {
      result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   }
   if (usage & TextureUsage::TransferDst) {
      result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
   }
   if (usage & TextureUsage::ColorAttachment) {
      result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   }
   if (usage & TextureUsage::DepthStencilAttachment) {
      result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   }
   if (usage & TextureUsage::Transient) {
      result |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
   }

   return result;
}

VkImageAspectFlags to_vulkan_image_aspect_flags(const TextureUsageFlags usage)
{
   if (usage & TextureUsage::DepthStencilAttachment) {
      return VK_IMAGE_ASPECT_DEPTH_BIT;
   }
   return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkBufferUsageFlags to_vulkan_buffer_usage_flags(const BufferUsageFlags usage)
{
   using enum BufferUsage;

   VkBufferUsageFlags result{};

   if (usage & TransferSrc) {
      result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   }
   if (usage & TransferDst) {
      result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
   }
   if (usage & UniformBuffer) {
      result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
   }
   if (usage & VertexBuffer) {
      result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
   }
   if (usage & IndexBuffer) {
      result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
   }
   if (usage & StorageBuffer) {
      result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
   }

   return result;
}

VkMemoryPropertyFlags to_vulkan_memory_properties_flags(BufferUsageFlags usage)
{
   using enum BufferUsage;

   VkMemoryPropertyFlags result{};
   if (usage & HostVisible) {
      result |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   } else {
      result |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   }

   return result;
}

VkPipelineStageFlags to_vulkan_wait_pipeline_stage(const WorkTypeFlags workTypes)
{
   using enum WorkType;

   if (workTypes & Graphics) {
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   }
   if (workTypes & Compute) {
      return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
   }
   if (workTypes & Transfer) {
      return VK_PIPELINE_STAGE_TRANSFER_BIT;
   }

   return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
}

VkPipelineBindPoint to_vulkan_pipeline_bind_point(PipelineType pipelineType)
{
   switch(pipelineType) {
   case PipelineType::Graphics:
      return VK_PIPELINE_BIND_POINT_GRAPHICS;
   case PipelineType::Compute:
      return VK_PIPELINE_BIND_POINT_COMPUTE;
   }
   return VK_PIPELINE_BIND_POINT_MAX_ENUM;
}

}// namespace triglav::graphics_api::vulkan
