#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

class Device;

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Buffer, Device);

namespace vulkan {
using DeviceMemory = WrappedObject<VkDeviceMemory, vkAllocateMemory, vkFreeMemory, VkDevice>;
}

class MappedMemory
{
 public:
   MappedMemory(void* pointer, VkDevice device, VkDeviceMemory deviceMemory);

   ~MappedMemory();

   MappedMemory(const MappedMemory& other) = delete;
   MappedMemory& operator=(const MappedMemory& other) = delete;

   MappedMemory(MappedMemory&& other) noexcept;
   MappedMemory& operator=(MappedMemory&& other) noexcept;

   [[nodiscard]] void* operator*() const;

   template<typename T>
   [[nodiscard]] T& cast() const
   {
      return *static_cast<T*>(m_pointer);
   }

   void write(const void* source, size_t length) const;
   void write_offset(const void* source, MemorySize length, MemorySize offset) const;

 private:
   void* m_pointer;
   VkDevice m_device;
   VkDeviceMemory m_deviceMemory;
};

using BufferAddress = VkDeviceAddress;

class Buffer
{
 public:
   Buffer(Device& device, VkDeviceSize m_size, vulkan::Buffer buffer, vulkan::DeviceMemory memory);

   Buffer(const Buffer& other) = delete;
   Buffer& operator=(const Buffer& other) = delete;
   Buffer(Buffer&& other) noexcept;
   Buffer& operator=(Buffer&& other) noexcept;


   [[nodiscard]] Status write_indirect(const void* data, size_t size);
   [[nodiscard]] size_t size() const;
   [[nodiscard]] BufferAddress buffer_address() const;

   [[nodiscard]] VkBuffer vulkan_buffer() const;
   [[nodiscard]] VkDeviceAddress vulkan_device_address() const;

   void set_debug_name(std::string_view name) const;

   Result<MappedMemory> map_memory();

 private:
   Device& m_device;
   VkDeviceSize m_size;
   vulkan::Buffer m_buffer;
   vulkan::DeviceMemory m_memory;
};

}// namespace triglav::graphics_api
