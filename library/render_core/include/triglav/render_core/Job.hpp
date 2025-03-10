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
      DescriptorStorage descStorage;
      std::vector<graphics_api::CommandList> commandList;
   };

   Job(graphics_api::Device& device, std::optional<graphics_api::DescriptorPool> descriptorPool, std::span<Frame> jobFrames,
       const graphics_api::WorkTypeFlags& workTypes, std::vector<Name> flags);

   void enable_flag(Name name);
   void disable_flag(Name name);

   void execute(u32 frameIndex, graphics_api::SemaphoreArrayView waitSemaphores, graphics_api::SemaphoreArrayView signalSemaphores,
                const graphics_api::Fence* fence) const;

 private:
   graphics_api::Device& m_device;
   std::optional<graphics_api::DescriptorPool> m_descriptorPool;
   std::array<Frame, FRAMES_IN_FLIGHT_COUNT> m_jobFrames;
   graphics_api::WorkTypeFlags m_workTypes;
   std::vector<Name> m_flags{};
   u32 m_enabledFlags{0};
};

}// namespace triglav::render_core
