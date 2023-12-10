#include "Texture.h"

#include "CommandList.h"
#include "Device.h"

namespace graphics_api {

Texture::Texture(vulkan::Image image, vulkan::DeviceMemory memory,
                 vulkan::ImageView imageView, const ColorFormat &colorFormat, const uint32_t width,
                 const uint32_t height) :
    m_image(std::move(image)),
    m_memory(std::move(memory)),
    m_colorFormat(colorFormat),
    m_imageView(std::move(imageView)),
    m_width{width},
    m_height{height}
{
}

VkImage Texture::vulkan_image() const
{
   return *m_image;
}

uint32_t Texture::width() const
{
   return m_width;
}

uint32_t Texture::height() const
{
   return m_height;
}

Status Texture::write(Device &device, const uint8_t *pixels) const
{
   const auto bufferSize = m_colorFormat.pixel_size() * m_width * m_height;
   auto transferBuffer   = device.create_buffer(BufferPurpose::TransferBuffer, bufferSize);
   if (not transferBuffer.has_value())
      return transferBuffer.error();

   {
      const auto mappedMemory = transferBuffer->map_memory();
      if (not mappedMemory.has_value())
         return mappedMemory.error();

      mappedMemory->write(pixels, bufferSize);
   }

   auto oneTimeCommands = device.create_command_list();
   if (not oneTimeCommands.has_value())
      return oneTimeCommands.error();

   if (const auto res = oneTimeCommands->begin_one_time(); res != Status::Success)
      return res;

   oneTimeCommands->copy_buffer_to_texture(*transferBuffer, *this);

   if (const auto res = oneTimeCommands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*oneTimeCommands); res != Status::Success)
      return res;

   return Status::Success;
}

VkImageView Texture::vulkan_image_view() const
{
   return *m_imageView;
}

}// namespace graphics_api
