#include "QueueManager.hpp"

#include "Device.hpp"

#include "triglav/Int.hpp"
#include "triglav/threading/Threading.hpp"

#include <stdexcept>

#include <spdlog/spdlog.h>

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
      m_queueGroups.emplace_back(std::make_unique<QueueGroup>(device, info));
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

QueueManager::SafeQueue& QueueManager::next_queue(const WorkTypeFlags flags)
{
   return this->queue_group(flags).next_queue();
}

std::pair<VkQueue, VkCommandPool> QueueManager::next_queue_and_command_pool(const WorkTypeFlags flags)
{
   auto& group = this->queue_group(flags);
   auto& queueAccessor = group.next_queue();
   return std::make_pair(*queueAccessor.access(), *group.command_pool());
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
   return m_semaphorePool.acquire_object();
}

void QueueManager::release_semaphore(const Semaphore* semaphore)
{
   m_semaphorePool.release_object(semaphore);
}

Fence* QueueManager::aquire_fence()
{
   return m_fencePool.acquire_object();
}

void QueueManager::release_fence(const Fence* fence)
{
   m_fencePool.release_object(fence);
}

QueueManager::QueueGroup::QueueGroup(Device& device, const QueueFamilyInfo& info) :
    m_device(device),
    m_queues(info.queueCount),
    m_flags(info.flags),
    m_queueFamilyIndex(info.index)
{
   for (u32 i = 0; i < info.queueCount; ++i) {
      auto& queue = m_queues[i];
      auto access = queue.access();
      vkGetDeviceQueue(device.vulkan_device(), info.index, i, &access.value());
      assert(*access);
   }

   const auto commandPoolCount = threading::total_thread_count();
   m_commandPools.reserve(commandPoolCount);
   std::generate_n(std::back_inserter(m_commandPools), commandPoolCount, [this] { return vulkan::CommandPool{m_device.vulkan_device()}; });

   VkCommandPoolCreateInfo commandPoolInfo{};
   commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   commandPoolInfo.queueFamilyIndex = info.index;

   // Construct a pool for every thread.
   for (auto& commandPool : m_commandPools) {
      if (const auto res = commandPool.construct(&commandPoolInfo); res != VK_SUCCESS) {
         throw std::runtime_error("failed to create command pool");
      }
   }
}

QueueManager::SafeQueue& QueueManager::QueueGroup::next_queue()
{
   auto& queue = m_queues[m_nextQueue];

   u32 nextQueue = m_nextQueue.load();
   while (not m_nextQueue.compare_exchange_strong(nextQueue, (nextQueue + 1) % m_queues.size()))
      ;

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
   allocateInfo.commandPool = *this->command_pool();
   allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocateInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   if (vkAllocateCommandBuffers(m_device.vulkan_device(), &allocateInfo, &commandBuffer) != VK_SUCCESS) {
      return std::unexpected(Status::UnsupportedDevice);
   }

   return CommandList(m_device, commandBuffer, *this->command_pool(), m_flags);
}

u32 QueueManager::QueueGroup::index() const
{
   return m_queueFamilyIndex;
}

const vulkan::CommandPool& QueueManager::QueueGroup::command_pool() const
{
   // Retrieve the pool assigned for this thread.
   return m_commandPools[threading::this_thread_id()];
}

QueueManager::QueueGroup& QueueManager::queue_group(const WorkTypeFlags type)
{
   const auto id = m_queueIndices[type.value];
   if (id >= m_queueGroups.size()) {
      throw std::runtime_error("unsupported queue type");
   }

   return *m_queueGroups[id];
}

const QueueManager::QueueGroup& QueueManager::queue_group(const WorkTypeFlags type) const
{
   const auto id = m_queueIndices[type.value];
   if (id >= m_queueGroups.size()) {
      throw std::runtime_error("unsupported queue type");
   }

   return *m_queueGroups[id];
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
