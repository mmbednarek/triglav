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
   std::map<Name, graphics_api::Semaphore> owned_semaphores;
   graphics_api::SemaphoreArray signal_semaphores;
   graphics_api::SemaphoreArray wait_semaphores;
   graphics_api::SemaphoreArray wait_in_frame_semaphores;
};
struct JobSemaphores
{
   std::array<JobFrameSemaphores, FRAMES_IN_FLIGHT_COUNT> frame_semaphores;
};

class JobGraph
{
 public:
   JobGraph(graphics_api::Device& device, resource::ResourceManager& resource_manager, PipelineCache& pipeline_cache,
            ResourceStorage& resource_storage, Vector2i screen_size);

   BuildContext& add_job(Name job_name);
   BuildContext& add_job(Name job_name, Vector2i screen_size);
   BuildContext& replace_job(Name job_name);
   BuildContext& replace_job(Name job_name, Vector2i screen_size);
   void add_external_job(Name job_name);
   void set_screen_size(Vector2i screen_size);

   void add_dependency(Name target, Name dependency);
   void add_dependency_to_previous_frame(Name target, Name dependency);
   void add_dependency_to_previous_frame(Name target_dep);
   void build_jobs(Name target_job);
   void rebuild_job(Name job);
   [[nodiscard]] graphics_api::Semaphore& semaphore(Name wait_job, Name signal_job, u32 frame_index);
   [[nodiscard]] graphics_api::SemaphoreArrayView wait_semaphores(Name wait_job, u32 frame_index);
   [[nodiscard]] graphics_api::SemaphoreArrayView signal_semaphores(Name signal_job, u32 frame_index);

   void enable_flag(Name job, Name flag);
   void disable_flag(Name job, Name flag);
   void execute(Name target_job, u32 frame_index, const graphics_api::Fence* fence);
   void build_semaphores();

   ResourceStorage& resources();

 private:
   void deduce_job_order(Name target_job);

   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;
   PipelineCache& m_pipeline_cache;
   ResourceStorage& m_resource_storage;
   Vector2i m_screen_size{};
   bool m_has_built_semaphores = false;
   bool m_is_first_frame = true;

   std::map<Name, BuildContext> m_contexts;
   std::map<Name, Job> m_jobs;
   std::map<Name, JobSemaphores> m_job_semaphores;
   std::vector<Name> m_external_jobs;
   std::multimap<Name, Name> m_dependencies;
   std::multimap<Name, Name> m_interframe_dependencies;
   std::vector<Name> m_job_order;
   std::optional<Name> m_last_target_job;
};

}// namespace triglav::render_core