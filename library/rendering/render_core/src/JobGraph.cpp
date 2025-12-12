#include "JobGraph.hpp"

#include "triglav/Ranges.hpp"

#include <deque>
#include <set>

namespace triglav::render_core {

JobGraph::JobGraph(graphics_api::Device& device, resource::ResourceManager& resource_manager, PipelineCache& pipeline_cache,
                   ResourceStorage& resource_storage, const Vector2i screen_size) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_pipeline_cache(pipeline_cache),
    m_resource_storage(resource_storage),
    m_screen_size(screen_size)
{
}

BuildContext& JobGraph::add_job(const Name job_name)
{
   return this->add_job(job_name, m_screen_size);
}

BuildContext& JobGraph::add_job(Name job_name, Vector2i screen_size)
{
   auto [build_ctx, ok] = m_contexts.emplace(job_name, BuildContext(m_device, m_resource_manager, screen_size));
   assert(ok);

   return build_ctx->second;
}

BuildContext& JobGraph::replace_job(const Name job_name)
{
   return this->replace_job(job_name, m_screen_size);
}

BuildContext& JobGraph::replace_job(Name job_name, Vector2i screen_size)
{
   m_contexts.erase(job_name);
   return this->add_job(job_name, screen_size);
}

void JobGraph::add_external_job(Name job_name)
{
   m_external_jobs.emplace_back(job_name);
}

void JobGraph::set_screen_size(const Vector2i screen_size)
{
   m_screen_size = screen_size;
}

void JobGraph::add_dependency(const Name target, const Name dependency)
{
   m_dependencies.emplace(target, dependency);
}

void JobGraph::add_dependency_to_previous_frame(const Name target, const Name dependency)
{
   m_interframe_dependencies.emplace(target, dependency);
}

void JobGraph::add_dependency_to_previous_frame(Name target_dep)
{
   this->add_dependency_to_previous_frame(target_dep, target_dep);
}

void JobGraph::enable_flag(const Name job, const Name flag)
{
   m_jobs.at(job).enable_flag(flag);
}

void JobGraph::disable_flag(const Name job, const Name flag)
{
   m_jobs.at(job).disable_flag(flag);
}

void JobGraph::build_jobs(const Name target_job)
{
   this->deduce_job_order(target_job);

   for (auto& name : m_job_order) {
      if (!m_contexts.contains(name))
         continue;

      auto& ctx = m_contexts.at(name);
      m_jobs.emplace(name, ctx.build_job(m_pipeline_cache, m_resource_storage, name));
      m_job_semaphores.emplace(name, JobSemaphores{});
   }
   for (const Name job_name : m_external_jobs) {
      m_job_semaphores.emplace(job_name, JobSemaphores{});
   }
}

void JobGraph::rebuild_job(const Name job)
{
   auto& ctx = m_contexts.at(job);
   m_jobs.erase(job);
   m_jobs.emplace(job, ctx.build_job(m_pipeline_cache, m_resource_storage, job));
}

graphics_api::Semaphore& JobGraph::semaphore(const Name wait_job, const Name signal_job, const u32 frame_index)
{
   return m_job_semaphores.at(wait_job).frame_semaphores[frame_index].owned_semaphores.at(signal_job);
}

graphics_api::SemaphoreArrayView JobGraph::wait_semaphores(const Name wait_job, const u32 frame_index)
{
   return m_job_semaphores.at(wait_job).frame_semaphores[frame_index].wait_semaphores;
}

graphics_api::SemaphoreArrayView JobGraph::signal_semaphores(const Name signal_job, const u32 frame_index)
{
   return m_job_semaphores.at(signal_job).frame_semaphores[frame_index].signal_semaphores;
}

void JobGraph::execute(const Name target_job, const u32 frame_index, const graphics_api::Fence* fence)
{
   this->build_semaphores();

   this->deduce_job_order(target_job);

   for (Name job_name : m_job_order) {
      if (!m_jobs.contains(job_name)) {
         continue;
      }

      auto& job = m_jobs.at(job_name);
      auto& job_semaphores = m_job_semaphores.at(job_name).frame_semaphores[frame_index];
      const auto* fence_ptr = job_name == target_job ? fence : nullptr;

      auto& wait_sems = m_is_first_frame ? job_semaphores.wait_in_frame_semaphores : job_semaphores.wait_semaphores;

      job.execute(frame_index, wait_sems, job_semaphores.signal_semaphores, fence_ptr);
   }

   m_is_first_frame = false;
}

ResourceStorage& JobGraph::resources()
{
   return m_resource_storage;
}

void JobGraph::build_semaphores()
{
   if (m_has_built_semaphores) {
      return;
   }

   for (auto& [job_name, semaphores] : m_job_semaphores) {
      auto& job_semaphores = semaphores.frame_semaphores;

      for (Name dependency : equal_range(m_dependencies, job_name)) {
         for (const u32 frame_index : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
            auto [sem, ok] = job_semaphores[frame_index].owned_semaphores.emplace(dependency, GAPI_CHECK(m_device.create_semaphore()));
            assert(ok);

            job_semaphores[frame_index].wait_semaphores.add_semaphore(sem->second);
            job_semaphores[frame_index].wait_in_frame_semaphores.add_semaphore(sem->second);

            auto& dep_semaphores = m_job_semaphores.at(dependency).frame_semaphores[frame_index];
            dep_semaphores.signal_semaphores.add_semaphore(sem->second);
         }
      }

      for (Name dependency : equal_range(m_interframe_dependencies, job_name)) {
         for (const u32 frame_index : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
            const u32 previous_frame_index = (frame_index + FRAMES_IN_FLIGHT_COUNT - 1) % FRAMES_IN_FLIGHT_COUNT;

            auto [sem, ok] = job_semaphores[frame_index].owned_semaphores.emplace(dependency, GAPI_CHECK(m_device.create_semaphore()));
            assert(ok);

            job_semaphores[frame_index].wait_semaphores.add_semaphore(sem->second);

            auto& dep_semaphores = m_job_semaphores.at(dependency).frame_semaphores[previous_frame_index];
            dep_semaphores.signal_semaphores.add_semaphore(sem->second);
         }
      }
   }

   m_has_built_semaphores = true;
}

void JobGraph::deduce_job_order(const Name target_job)
{
   if (m_last_target_job.has_value() && *m_last_target_job == target_job) {
      return;
   }

   m_job_order.clear();
   m_last_target_job = target_job;

   std::deque<Name> job_queue;
   job_queue.emplace_front(target_job);

   std::set<Name> visited;
   std::set<Name> inserted;

   while (!job_queue.empty()) {
      Name job = job_queue.front();

      if (visited.contains(job)) {
         job_queue.pop_front();

         if (!inserted.contains(job)) {
            m_job_order.emplace_back(job);
            inserted.emplace(job);
         }
         continue;
      }

      visited.emplace(job);

      bool all_satisfied = true;
      for (Name dependency : equal_range(m_dependencies, job)) {
         if (!inserted.contains(dependency)) {
            job_queue.emplace_front(dependency);
            all_satisfied = false;
         }
      }

      if (all_satisfied) {
         job_queue.pop_front();

         if (!inserted.contains(job)) {
            m_job_order.emplace_back(job);
            inserted.emplace(job);
         }
      }
   }
}

}// namespace triglav::render_core