#include "Synchronization.hpp"

namespace triglav::graphics_api {

Fence::Fence(vulkan::Fence fence) :
    m_fence(std::move(fence))
{
}

VkFence Fence::vulkan_fence() const
{
   return *m_fence;
}

void Fence::await() const
{
   vkWaitForFences(m_fence.parent(), 1, &(*m_fence), true, UINT64_MAX);
   vkResetFences(m_fence.parent(), 1, &(*m_fence));
}

Semaphore::Semaphore(vulkan::Semaphore semaphore) :
    m_semaphore(std::move(semaphore))
{
}

VkSemaphore Semaphore::vulkan_semaphore() const
{
   return *m_semaphore;
}

const VkSemaphore* SemaphoreArray::vulkan_semaphores() const
{
   return m_semaphores.data();
}

size_t SemaphoreArray::semaphore_count() const
{
   return m_semaphores.size();
}

void SemaphoreArray::add_semaphore(const Semaphore& semaphore)
{
   m_semaphores.emplace_back(semaphore.vulkan_semaphore());
}

SemaphoreArrayView::SemaphoreArrayView() :
    m_semaphores(nullptr),
    m_count(0)
{
}

SemaphoreArrayView::SemaphoreArrayView(const SemaphoreArray& array) :
    m_semaphores(array.vulkan_semaphores()),
    m_count(array.semaphore_count())
{
}

SemaphoreArrayView::SemaphoreArrayView(const SemaphoreArray& array, size_t count) :
    m_semaphores(array.vulkan_semaphores()),
    m_count(std::min(count, array.semaphore_count()))
{
}

const VkSemaphore* SemaphoreArrayView::vulkan_semaphores() const
{
   return m_semaphores;
}

size_t SemaphoreArrayView::semaphore_count() const
{
   return m_count;
}

}// namespace triglav::graphics_api
