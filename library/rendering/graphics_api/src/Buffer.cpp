#include "Buffer.hpp"

#include "CommandList.hpp"
#include "Device.hpp"
#include "ReplicatedBuffer.hpp"

#include <cstring>

namespace triglav::graphics_api {

MappedMemory::MappedMemory(void* pointer, const VkDevice device, const VkDeviceMemory device_memory) :
    m_pointer(pointer),
    m_device(device),
    m_device_memory(device_memory)
{
}

MappedMemory::~MappedMemory()
{
   if (m_device != nullptr && m_device_memory != nullptr) {
      vkUnmapMemory(m_device, m_device_memory);
   }
}

MappedMemory::MappedMemory(MappedMemory&& other) noexcept :
    m_pointer(std::exchange(other.m_pointer, nullptr)),
    m_device(std::exchange(other.m_device, nullptr)),
    m_device_memory(std::exchange(other.m_device_memory, nullptr))
{
}

MappedMemory& MappedMemory::operator=(MappedMemory&& other) noexcept
{
   if (this == &other)
      return *this;

   m_pointer = std::exchange(other.m_pointer, nullptr);
   m_device = std::exchange(other.m_device, nullptr);
   m_device_memory = std::exchange(other.m_device_memory, nullptr);

   return *this;
}

void* MappedMemory::operator*() const
{
   return m_pointer;
}

void MappedMemory::write(const void* source, const size_t length) const
{
   std::memcpy(m_pointer, source, length);
}

void MappedMemory::write_offset(const void* source, const MemorySize length, const MemorySize offset) const
{
   std::memcpy(static_cast<u8*>(m_pointer) + offset, source, length);
}

Buffer::Buffer(Device& device, VkDeviceSize size, vulkan::Buffer buffer, vulkan::DeviceMemory memory) :
    m_device(device),
    m_size(size),
    m_buffer(std::move(buffer)),
    m_memory(std::move(memory))
{
}

Buffer::Buffer(Buffer&& other) noexcept :
    m_device(other.m_device),
    m_size(std::exchange(other.m_size, 0)),
    m_buffer(std::move(other.m_buffer)),
    m_memory(std::move(other.m_memory))
{
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
   m_size = std::exchange(other.m_size, 0);
   m_buffer = std::move(other.m_buffer);
   m_memory = std::move(other.m_memory);
   return *this;
}

Result<MappedMemory> Buffer::map_memory()
{
   void* pointer;
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

Status Buffer::write_indirect(const void* data, size_t size)
{
   auto transfer_buffer = m_device.create_buffer(BufferUsage::HostVisible | BufferUsage::TransferSrc, size);
   if (not transfer_buffer.has_value())
      return transfer_buffer.error();

   {
      const auto mapped_memory = transfer_buffer->map_memory();
      if (not mapped_memory.has_value())
         return mapped_memory.error();

      mapped_memory->write(data, size);
   }

   auto one_time_commands = m_device.create_command_list();
   if (not one_time_commands.has_value())
      return one_time_commands.error();

   if (const auto res = one_time_commands->begin(SubmitType::OneTime); res != Status::Success)
      return res;

   one_time_commands->copy_buffer(*transfer_buffer, *this);

   if (const auto res = one_time_commands->finish(); res != Status::Success)
      return res;

   if (const auto res = m_device.submit_command_list_one_time(*one_time_commands); res != Status::Success)
      return res;

   return Status::Success;
}

VkDeviceAddress Buffer::vulkan_device_address() const
{
   VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
   info.buffer = *m_buffer;
   return vkGetBufferDeviceAddress(m_device.vulkan_device(), &info);
}

void Buffer::set_debug_name(const std::string_view name) const
{
   if (name.empty())
      return;

   VkDebugUtilsObjectNameInfoEXT debug_utils_object_name{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(*m_buffer);
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_BUFFER;
   debug_utils_object_name.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_buffer.parent(), &debug_utils_object_name);
   assert(result == VK_SUCCESS);
}

BufferAddress Buffer::buffer_address() const
{
   return BufferAddress{this->vulkan_device_address()};
}

}// namespace triglav::graphics_api
