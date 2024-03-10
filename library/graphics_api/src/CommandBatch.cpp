#include "CommandBatch.h"

#include "Device.h"

namespace triglav::graphics_api {

void CommandBatch::add_command_list(const WorkType type, CommandList &list)
{
   switch(type) {
   case WorkType::Graphics:
      m_graphicCommands.emplace_back(&list);
      break;
   case WorkType::Transfer:
      m_transferCommands.emplace_back(&list);
      break;
   default: break;
   }
}

// void CommandBatch::submit_worktype(const WorkType type, const std::vector<CommandList*>& commands, SemaphorePool& semaphorePool, FencePool& fencePool) const
// {
//    const auto queueCount = std::min(commands.size(), m_device.queue_count(type));
//    const auto commandsRemainder = commands.size() % queueCount;
//    auto commandsPerQueue = commands.size() / queueCount;
//    if (commandsRemainder != 0) {
//       ++commandsPerQueue;
//    }
//
//    std::vector<VkCommandBuffer> commandBuffers{};
//    commandBuffers.resize(commandsPerQueue);
//
//    int commandIndex = 0;
//
//    std::vector<Semaphore*> semaphores{};
//    semaphores.resize(queueCount);
//    std::vector<Fence*> fences{};
//    semaphores.resize(queueCount);
//
//    for (int i = 0; i < queueCount; ++i) {
//       const auto commandsCount = i <= commandsRemainder ? commandsPerQueue : (commandsPerQueue - 1);
//       if (commandsCount == 0) {
//          break;
//       }
//
//       for (int j = 0; j < commandsCount; ++j) {
//          commandBuffers[j] = commands[commandIndex]->vulkan_command_buffer();
//          ++commandIndex;
//       }
//
//       auto semaphore = semaphorePool.aquire_object();
//       auto vulkan_semaphore = semaphore->vulkan_semaphore();
//       auto fence = fences.emplace_back(fencePool.aquire_object())->vulkan_fence();
//
//       VkQueue queue = m_device.queue(type, i);
//       VkSubmitInfo info{};
//       info.commandBufferCount = commandsCount;
//       info.pCommandBuffers = commandBuffers.data();
//       info.waitSemaphoreCount = m_waitSemaphores.semaphore_count();
//       info.pWaitSemaphores = m_waitSemaphores.vulkan_semaphores();
//       info.pSignalSemaphores = &semaphore;
//       info.signalSemaphoreCount = 1;
//       vkQueueSubmit(queue, 1, &info, fence);
//    }
// }

}// namespace triglav::graphics_api