#pragma once

#include "CommandList.h"
#include "Synchronization.h"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/ObjectPool.hpp"
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
   u32 queueCount{};
   WorkTypeFlags flags{WorkType::None};
};

class QueueManager
{
 public:
   using SafeQueue = threading::SafeAccess<VkQueue>;

   explicit QueueManager(Device& device, std::span<QueueFamilyInfo> infos);

   [[nodiscard]] SafeQueue& next_queue(WorkTypeFlags flags);
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags) const;
   [[nodiscard]] u32 queue_index(WorkTypeFlags flags) const;
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

    private:
      Device& m_device;
      std::vector<SafeQueue> m_queues;
      WorkTypeFlags m_flags;
      std::vector<vulkan::CommandPool> m_commandPools;
      u32 m_queueFamilyIndex{};
      mutable std::atomic<u32> m_nextQueue;
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

   Device& m_device;
   SemaphoreFactory m_semaphoreFactory;
   ObjectPool<Semaphore, SemaphoreFactory, 8> m_semaphorePool;
   FenceFactory m_fenceFactory;
   ObjectPool<Fence, FenceFactory, 4> m_fencePool;
   std::vector<std::unique_ptr<QueueGroup>> m_queueGroups;
   std::array<u32, 16> m_queueIndices{};
};

}// namespace triglav::graphics_api
