#include "QueueManager.h"

#include "Device.h"

#include <stdexcept>

namespace triglav::graphics_api {

namespace {

u32 work_type_flag_count(const WorkTypeFlags flags)
{
   u32 result{};
   if (flags & WorkType::Graphics)
      ++result;
   if (flags & WorkType::Compute)
      ++result;
   if (flags & WorkType::Transfer)
      ++result;
   if (flags & WorkType::Presentation)
      ++result;
   return result;
}

}// namespace

QueueManager::QueueManager(Device& device, std::span<QueueFamilyInfo> infos) :
    m_device(device),
    m_semaphoreFactory(device),
    m_semaphorePool(m_semaphoreFactory),
    m_fenceFactory(device),
    m_fencePool(m_fenceFactory)
{
   m_queueIndices.fill(std::numeric_limits<u32>::max());

   std::array<u32, 16> queueCounts{};
   queueCounts.fill(std::numeric_limits<u32>::max());

   u32 index{};
   for (const auto& info : infos) {
      m_queueGroups.emplace_back(device, info);
      for (u32 flagsInt = 1; flagsInt < 16; ++flagsInt) {
         const WorkTypeFlags flags{flagsInt};
         const auto flagCount = work_type_flag_count(flags);
         if (info.flags & flags && queueCounts[flagsInt] >= flagCount) {
            queueCounts[flagsInt] = flagCount;
            m_queueIndices[flagsInt] = index;
         }
      }
      ++index;
   }
}

VkQueue QueueManager::next_queue(const WorkTypeFlags flags) const
{
   return this->queue_group(flags).next_queue();
}

Result<CommandList> QueueManager::create_command_list(const WorkTypeFlags flags) const
{
   return this->queue_group(flags).create_command_list();
}

u32 QueueManager::queue_index(const WorkTypeFlags flags) const
{
   return this->queue_group(flags).index();
}

Semaphore* QueueManager::aquire_semaphore()
{
   return m_semaphorePool.aquire_object();
}

void QueueManager::release_semaphore(const Semaphore* semaphore)
{
   m_semaphorePool.release_object(semaphore);
}

Fence* QueueManager::aquire_fence()
{
   return m_fencePool.aquire_object();
}

void QueueManager::release_fence(const Fence* fence)
{
   m_fencePool.release_object(fence);
}

QueueManager::QueueGroup::QueueGroup(Device& device, const QueueFamilyInfo& info) :
    m_device(device),
    m_flags(info.flags),
    m_commandPool(device.vulkan_device()),
    m_queueFamilyIndex(info.index)
{
   m_queues.resize(info.queueCount);
   for (u32 i = 0; i < info.queueCount; ++i) {
      vkGetDeviceQueue(device.vulkan_device(), info.index, i, &m_queues[i]);
      assert(m_queues[i]);
   }

   VkCommandPoolCreateInfo commandPoolInfo{};
   commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   commandPoolInfo.queueFamilyIndex = info.index;
   if (const auto res = m_commandPool.construct(&commandPoolInfo); res != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool");
   }
}

VkQueue QueueManager::QueueGroup::next_queue() const
{
   const auto queue = m_queues[m_nextQueue];
   m_nextQueue = (m_nextQueue + 1) % m_queues.size();
   return queue;
}

WorkTypeFlags QueueManager::QueueGroup::flags() const
{
   return m_flags;
}

Result<CommandList> QueueManager::QueueGroup::create_command_list() const
{
   VkCommandBufferAllocateInfo allocateInfo{};
   allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocateInfo.commandPool = *m_commandPool;
   allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocateInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   if (vkAllocateCommandBuffers(m_device.vulkan_device(), &allocateInfo, &commandBuffer) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return CommandList(m_device, commandBuffer, *m_commandPool, m_flags);
}

u32 QueueManager::QueueGroup::index() const
{
   return m_queueFamilyIndex;
}

QueueManager::QueueGroup& QueueManager::queue_group(const WorkTypeFlags type)
{
   const auto id = m_queueIndices[type.value];
   if (id >= m_queueGroups.size()) {
      throw std::runtime_error("unsupported queue type");
   }

   return m_queueGroups[id];
}

const QueueManager::QueueGroup& QueueManager::queue_group(const WorkTypeFlags type) const
{
   const auto id = m_queueIndices[type.value];
   if (id >= m_queueGroups.size()) {
      throw std::runtime_error("unsupported queue type");
   }

   return m_queueGroups[id];
}

QueueManager::SemaphoreFactory::SemaphoreFactory(Device& device) :
    m_device(device)
{
}

Semaphore QueueManager::SemaphoreFactory::operator()() const
{
   return GAPI_CHECK(m_device.create_semaphore());
}

QueueManager::FenceFactory::FenceFactory(Device& device) :
    m_device(device)
{
}

Fence QueueManager::FenceFactory::operator()() const
{
   return GAPI_CHECK(m_device.create_fence());
}

}// namespace triglav::graphics_api
