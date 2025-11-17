#include "Job.hpp"

#include "triglav/Template.hpp"

namespace triglav::render_core {

Job::Job(graphics_api::Device& device, std::optional<graphics_api::DescriptorPool> descriptor_pool, const std::span<Frame> job_frames,
         const graphics_api::WorkTypeFlags& work_types, std::vector<Name> flags) :
    m_device(device),
    m_descriptor_pool(std::move(descriptor_pool)),
    m_job_frames(span_to_array<Frame, FRAMES_IN_FLIGHT_COUNT>(job_frames)),
    m_work_types(work_types),
    m_flags(std::move(flags))
{
}

void Job::enable_flag(const Name name)
{
   const auto index = std::ranges::find(m_flags, name) - m_flags.begin();
   m_enabled_flags |= (1 << index);
}

void Job::disable_flag(const Name name)
{
   const auto index = std::ranges::find(m_flags, name) - m_flags.begin();
   m_enabled_flags &= ~(1 << index);
}

void Job::execute(const u32 frame_index, const graphics_api::SemaphoreArrayView wait_semaphores,
                  const graphics_api::SemaphoreArrayView signal_semaphores, const graphics_api::Fence* fence) const
{
   assert(frame_index < FRAMES_IN_FLIGHT_COUNT);
   GAPI_CHECK_STATUS(m_device.submit_command_list(m_job_frames.at(frame_index).command_list.at(m_enabled_flags), wait_semaphores,
                                                  signal_semaphores, fence, m_work_types));
}

}// namespace triglav::render_core
