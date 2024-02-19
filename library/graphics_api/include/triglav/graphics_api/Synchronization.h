#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Fence, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(Semaphore, Device)

class Fence
{
 public:
   explicit Fence(vulkan::Fence fence);

   [[nodiscard]] VkFence vulkan_fence() const;
   void await() const;

 private:
   vulkan::Fence m_fence;
};

class Semaphore
{
 public:
   explicit Semaphore(vulkan::Semaphore semaphore);

   [[nodiscard]] VkSemaphore vulkan_semaphore() const;

 private:
   vulkan::Semaphore m_semaphore;
};

}// namespace triglav::graphics_api
