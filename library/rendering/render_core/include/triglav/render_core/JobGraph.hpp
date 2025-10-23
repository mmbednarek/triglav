#pragma once

#include "BuildContext.hpp"
#include "Job.hpp"

#include "triglav/graphics_api/Synchronization.hpp"

#include <map>

namespace triglav::graphics_api {
class Device;
class Fence;
class SemaphoreArray;
}// namespace triglav::graphics_api

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::render_core {

class ResourceStorage;
class PipelineCache;

struct JobFrameSemaphores
{
   std::map<Name, graphics_api::Semaphore> ownedSemaphores;
   graphics_api::SemaphoreArray signalSemaphores;
   graphics_api::SemaphoreArray waitSemaphores;
   graphics_api::SemaphoreArray waitInFrameSemaphores;
};
struct JobSemaphores
{
   std::array<JobFrameSemaphores, FRAMES_IN_FLIGHT_COUNT> frameSemaphores;
};

class JobGraph
{
 public:
   JobGraph(graphics_api::Device& device, resource::ResourceManager& resourceManager, PipelineCache& pipelineCache,
            ResourceStorage& resourceStorage, Vector2i screenSize);

   BuildContext& add_job(Name jobName);
   BuildContext& add_job(Name jobName, Vector2i screenSize);
   BuildContext& replace_job(Name jobName);
   BuildContext& replace_job(Name jobName, Vector2i screenSize);
   void add_external_job(Name jobName);
   void set_screen_size(Vector2i screenSize);

   void add_dependency(Name target, Name dependency);
   void add_dependency_to_previous_frame(Name target, Name dependency);
   void add_dependency_to_previous_frame(Name targetDep);
   void build_jobs(Name targetJob);
   void rebuild_job(Name job);
   [[nodiscard]] graphics_api::Semaphore& semaphore(Name waitJob, Name signalJob, u32 frameIndex);
   [[nodiscard]] graphics_api::SemaphoreArrayView wait_semaphores(Name waitJob, u32 frameIndex);
   [[nodiscard]] graphics_api::SemaphoreArrayView signal_semaphores(Name signalJob, u32 frameIndex);

   void enable_flag(Name job, Name flag);
   void disable_flag(Name job, Name flag);
   void execute(Name targetJob, u32 frameIndex, const graphics_api::Fence* fence);
   void build_semaphores();

   ResourceStorage& resources();

 private:
   void deduce_job_order(Name targetJob);

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   PipelineCache& m_pipelineCache;
   ResourceStorage& m_resourceStorage;
   Vector2i m_screenSize{};
   bool m_hasBuiltSemaphores = false;
   bool m_isFirstFrame = true;

   std::map<Name, BuildContext> m_contexts;
   std::map<Name, Job> m_jobs;
   std::map<Name, JobSemaphores> m_jobSemaphores;
   std::vector<Name> m_externalJobs;
   std::multimap<Name, Name> m_dependencies;
   std::multimap<Name, Name> m_interframeDependencies;
   std::vector<Name> m_jobOrder;
   std::optional<Name> m_lastTargetJob;
};

}// namespace triglav::render_core