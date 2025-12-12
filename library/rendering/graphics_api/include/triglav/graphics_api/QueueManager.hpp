#pragma once

#include "CommandList.hpp"
#include "Synchronization.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/ObjectPool.hpp"
#include "triglav/ktx/Vulkan.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <atomic>
#include <mutex>
#include <span>
#include <vector>

namespace triglav::graphics_api {

class Device;

struct QueueFamilyInfo
{
   u32 index{};
   u32 queue_count{};
   WorkTypeFlags flags{WorkType::None};
};

class QueueManager
{
 public:
   using SafeQueue = threading::SafeAccess<VkQueue>;

   QueueManager(Device& device, std::span<QueueFamilyInfo> infos);

   [[nodiscard]] SafeQueue& next_queue(WorkTypeFlags flags);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags) const;
   [[nodiscard]] u32 queue_index(WorkTypeFlags flags) const;
   [[nodiscard]] std::pair<SafeQueue&, ktx::VulkanDeviceInfo&> ktx_device_info();
   [[nodiscard]] Semaphore* aquire_semaphore();
   void release_semaphore(const Semaphore* semaphore);
   [[nodiscard]] Fence* aquire_fence();
   void release_fence(const Fence* fence);

 private:
   class QueueGroup
   {
    public:
      QueueGroup(Device& device, const QueueFamilyInfo& info);
      SafeQueue& next_queue();
      [[nodiscard]] WorkTypeFlags flags() const;
      [[nodiscard]] Result<CommandList> create_command_list() const;
      [[nodiscard]] u32 index() const;
      [[nodiscard]] const vulkan::CommandPool& command_pool() const;
      std::pair<SafeQueue&, ktx::VulkanDeviceInfo&> ktx_device_info();

    private:
      Device& m_device;
      std::vector<SafeQueue> m_queues;
      WorkTypeFlags m_flags;
      std::vector<vulkan::CommandPool> m_command_pools;
      u32 m_queue_family_index{};
      mutable std::atomic<u32> m_next_queue;
      std::vector<ktx::VulkanDeviceInfo> m_ktxDeviceInfos;
   };

   class SemaphoreFactory
   {
    public:
      explicit SemaphoreFactory(Device& device);

      Semaphore operator()() const;

    private:
      Device& m_device;
   };

   class FenceFactory
   {
    public:
      explicit FenceFactory(Device& device);

      Fence operator()() const;

    private:
      Device& m_device;
   };

   [[nodiscard]] QueueGroup& queue_group(WorkTypeFlags type);
   [[nodiscard]] const QueueGroup& queue_group(WorkTypeFlags type) const;

   SemaphoreFactory m_semaphore_factory;
   ObjectPool<Semaphore, SemaphoreFactory, 8> m_semaphore_pool;
   FenceFactory m_fence_factory;
   ObjectPool<Fence, FenceFactory, 4> m_fence_pool;
   std::vector<std::unique_ptr<QueueGroup>> m_queue_groups;
   std::array<u32, 16> m_queue_indices{};
};

}// namespace triglav::graphics_api
