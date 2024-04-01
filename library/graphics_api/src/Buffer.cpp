#include "Buffer.h"

#include "CommandList.h"
#include "Device.h"

#include <cstring>

namespace triglav::graphics_api {

MappedMemory::MappedMemory(void *pointer, const VkDevice device, const VkDeviceMemory deviceMemory) :
    m_pointer(pointer),
    m_device(device),
    m_deviceMemory(deviceMemory)
{
}

MappedMemory::~MappedMemory()
{
   if (m_device != nullptr && m_deviceMemory != nullptr) {
      vkUnmapMemory(m_device, m_deviceMemory);
   }
}

MappedMemory::MappedMemory(MappedMemory &&other) noexcept :
    m_pointer(std::exchange(other.m_pointer, nullptr)),
    m_device(std::exchange(other.m_device, nullptr)),
    m_deviceMemory(std::exchange(other.m_deviceMemory, nullptr))
{
}

MappedMemory &MappedMemory::operator=(MappedMemory &&other) noexcept
{
   if (this == &other)
      return *this;

   m_pointer      = std::exchange(other.m_pointer, nullptr);
   m_device       = std::exchange(other.m_device, nullptr);
   m_deviceMemory = std::exchange(other.m_deviceMemory, nullptr);

   return *this;
}

void *MappedMemory::operator*() const
{
   return m_pointer;
}

void MappedMemory::write(const void *source, const size_t length) const
{
   std::memcpy(m_pointer, source, length);
}

Buffer::Buffer(VkDeviceSize size, vulkan::Buffer buffer, vulkan::DeviceMemory memory) :
    m_size(size),
    m_buffer(std::move(buffer)),
    m_memory(std::move(memory))
{
}

Result<MappedMemory> Buffer::map_memory()
{
   void *pointer;
   if (vkMapMemory(m_memory.parent(), *m_memory, 0, m_size, 0, &pointer) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return MappedMemory(pointer, m_memory.parent(), *m_memory);
}

VkBuffer Buffer::vulkan_buffer() const
{
   return *m_buffer;
}

size_t Buffer::size() const
{
   return m_size;
}

Status write_to_buffer(Device &device, const Buffer &buffer, const void *data, const size_t size)
{
   auto transferBuffer = device.create_buffer(BufferPurpose::TransferBuffer, size);
   if (not transferBuffer.has_value())
      return transferBuffer.error();

   {
      const auto mappedMemory = transferBuffer->map_memory();
      if (not mappedMemory.has_value())
         return mappedMemory.error();

      mappedMemory->write(data, size);
   }

   auto oneTimeCommands = device.create_command_list();
   if (not oneTimeCommands.has_value())
      return oneTimeCommands.error();

   if (const auto res = oneTimeCommands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   oneTimeCommands->copy_buffer(*transferBuffer, buffer);

   if (const auto res = oneTimeCommands->finish(); res != Status::Success)
      return res;

   if (const auto res = device.submit_command_list_one_time(*oneTimeCommands); res != Status::Success)
      return res;

   return Status::Success;
}

}// namespace graphics_api
