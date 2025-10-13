#include "Job.hpp"

#include "triglav/Template.hpp"

namespace triglav::render_core {

Job::Job(graphics_api::Device& device, std::optional<graphics_api::DescriptorPool> descriptorPool, const std::span<Frame> jobFrames,
         const graphics_api::WorkTypeFlags& workTypes, std::vector<Name> flags) :
    m_device(device),
    m_descriptorPool(std::move(descriptorPool)),
    m_jobFrames(span_to_array<Frame, FRAMES_IN_FLIGHT_COUNT>(jobFrames)),
    m_workTypes(workTypes),
    m_flags(std::move(flags))
{
}

void Job::enable_flag(const Name name)
{
   const auto index = std::ranges::find(m_flags, name) - m_flags.begin();
   m_enabledFlags |= (1 << index);
}

void Job::disable_flag(const Name name)
{
   const auto index = std::ranges::find(m_flags, name) - m_flags.begin();
   m_enabledFlags &= ~(1 << index);
}

void Job::execute(const u32 frameIndex, const graphics_api::SemaphoreArrayView waitSemaphores,
                  const graphics_api::SemaphoreArrayView signalSemaphores, const graphics_api::Fence* fence) const
{
   assert(frameIndex < FRAMES_IN_FLIGHT_COUNT);
   GAPI_CHECK_STATUS(m_device.submit_command_list(m_jobFrames.at(frameIndex).commandList.at(m_enabledFlags), waitSemaphores,
                                                  signalSemaphores, fence, m_workTypes));
}

}// namespace triglav::render_core
