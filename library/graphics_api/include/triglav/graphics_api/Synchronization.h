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

class SemaphoreArray
{
 public:
   [[nodiscard]] const VkSemaphore *vulkan_semaphores() const;
   [[nodiscard]] size_t semaphore_count() const;

   void add_semaphore(const Semaphore &semaphore);

 private:
   std::vector<VkSemaphore> m_semaphores;
};

class SemaphoreArrayView {
 public:
   SemaphoreArrayView();
   SemaphoreArrayView(const SemaphoreArray& array);
   SemaphoreArrayView(const SemaphoreArray& array, size_t count);

   [[nodiscard]] const VkSemaphore *vulkan_semaphores() const;
   [[nodiscard]] size_t semaphore_count() const;

 private:
   const VkSemaphore* m_semaphores;
   size_t m_count;
};

}// namespace triglav::graphics_api
