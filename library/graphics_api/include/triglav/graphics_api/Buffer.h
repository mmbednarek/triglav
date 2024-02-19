#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

class Device;

enum class BufferPurpose
{
   TransferBuffer,
   VertexBuffer,
   UniformBuffer,
   IndexBuffer
};

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Buffer, Device);

namespace vulkan {
using DeviceMemory = WrappedObject<VkDeviceMemory, vkAllocateMemory, vkFreeMemory, VkDevice>;
}

class MappedMemory
{
 public:
   MappedMemory(void *pointer, VkDevice device, VkDeviceMemory deviceMemory);

   ~MappedMemory();

   MappedMemory(const MappedMemory &other)            = delete;
   MappedMemory &operator=(const MappedMemory &other) = delete;

   MappedMemory(MappedMemory &&other) noexcept;
   MappedMemory &operator=(MappedMemory &&other) noexcept;

   [[nodiscard]] void *operator*() const;

   void write(const void *source, size_t length) const;

 private:
   void *m_pointer;
   VkDevice m_device;
   VkDeviceMemory m_deviceMemory;
};

class Buffer
{
 public:
   explicit Buffer(VkDeviceSize m_size, vulkan::Buffer buffer, vulkan::DeviceMemory memory);

   [[nodiscard]] VkBuffer vulkan_buffer() const;
   Result<MappedMemory> map_memory();
   [[nodiscard]] size_t size() const;

 private:
   VkDeviceSize m_size;
   vulkan::Buffer m_buffer;
   vulkan::DeviceMemory m_memory;
};

Status write_to_buffer(Device& device, const Buffer & buffer, const void * data, size_t size);

}// namespace graphics_api
