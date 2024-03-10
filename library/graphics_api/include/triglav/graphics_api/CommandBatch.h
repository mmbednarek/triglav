#pragma once

#include "CommandList.h"
#include "GraphicsApi.hpp"
#include "Synchronization.h"

#include "triglav/ObjectPool.hpp"

namespace triglav::graphics_api {

class Device;

class CommandBatch {
public:
    CommandBatch(const SemaphoreArray& waitSemaphores);

    void add_command_list(WorkType type, CommandList& list);
    const SemaphoreArray& submit();

private:
    // void submit_worktype(WorkType type, const std::vector<CommandList*>& commands, SemaphorePool& semaphorePool, FencePool& fencePool) const;

    Device& m_device;
    const SemaphoreArray& m_waitSemaphores;
    SemaphoreArray m_signalSemaphores;
    std::vector<CommandList*> m_graphicCommands;
    std::vector<CommandList*> m_transferCommands;
};

}