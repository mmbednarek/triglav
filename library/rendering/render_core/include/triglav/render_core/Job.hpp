#pragma once

#include "RenderCore.hpp"
#include "ResourceStorage.hpp"

#include "triglav/graphics_api/CommandList.hpp"

#include <array>

namespace triglav::render_core {

class Job
{
 public:
   struct Frame
   {
      DescriptorStorage desc_storage;
      std::vector<graphics_api::CommandList> command_list;
   };

   Job(graphics_api::Device& device, std::optional<graphics_api::DescriptorPool> descriptor_pool, std::span<Frame> job_frames,
       const graphics_api::WorkTypeFlags& work_types, std::vector<Name> flags);

   void enable_flag(Name name);
   void disable_flag(Name name);

   void execute(u32 frame_index, graphics_api::SemaphoreArrayView wait_semaphores, graphics_api::SemaphoreArrayView signal_semaphores,
                const graphics_api::Fence* fence) const;

 private:
   graphics_api::Device& m_device;
   std::optional<graphics_api::DescriptorPool> m_descriptor_pool;
   std::array<Frame, FRAMES_IN_FLIGHT_COUNT> m_job_frames;
   graphics_api::WorkTypeFlags m_work_types;
   std::vector<Name> m_flags{};
   u32 m_enabled_flags{0};
};

}// namespace triglav::render_core
