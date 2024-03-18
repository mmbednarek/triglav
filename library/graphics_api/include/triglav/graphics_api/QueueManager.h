#pragma once

#include "CommandList.h"
#include "Synchronization.h"
#include "vulkan/ObjectWrapper.hpp"

#include "triglav/ObjectPool.hpp"

#include <span>
#include <vector>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(CommandPool, Device)

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
   explicit QueueManager(Device &device, std::span<QueueFamilyInfo> infos);

   [[nodiscard]] VkQueue next_queue(WorkTypeFlags flags) const;
   [[nodiscard]] Result<CommandList> create_command_list(WorkTypeFlags flags) const;
   [[nodiscard]] u32 queue_index(WorkTypeFlags flags) const;
   [[nodiscard]] Semaphore* aquire_semaphore();
   void release_semaphore(const Semaphore *semaphore);
   [[nodiscard]] Fence* aquire_fence();
   void release_fence(const Fence *fence);

 private:
   class QueueGroup
   {
    public:
      QueueGroup(const Device &device, const QueueFamilyInfo &info);
      VkQueue next_queue() const;
      [[nodiscard]] WorkTypeFlags flags() const;
      [[nodiscard]] Result<CommandList> create_command_list() const;
      [[nodiscard]] u32 index() const;

    private:
      VkDevice m_vulkanDevice;
      std::vector<VkQueue> m_queues;
      WorkTypeFlags m_flags;
      vulkan::CommandPool m_commandPool;
      u32 m_queueFamilyIndex{};
      mutable u32 m_nextQueue{};
   };

   class SemaphoreFactory
   {
    public:
      explicit SemaphoreFactory(Device &device);

      Semaphore operator()() const;

    private:
      Device &m_device;
   };

   class FenceFactory
   {
    public:
      explicit FenceFactory(Device &device);

      Fence operator()() const;

    private:
      Device &m_device;
   };

   QueueGroup &queue_group(WorkTypeFlags type);
   const QueueGroup &queue_group(WorkTypeFlags type) const;

   SemaphoreFactory m_semaphoreFactory;
   ObjectPool<Semaphore, SemaphoreFactory> m_semaphorePool;
   FenceFactory m_fenceFactory;
   ObjectPool<Fence, FenceFactory> m_fencePool;
   std::vector<QueueGroup> m_queueGroups;
   std::array<u32, 16> m_queueIndices{};
};

}// namespace triglav::graphics_api
