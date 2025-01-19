#include "JobGraph.hpp"

#include "triglav/Ranges.hpp"

#include <deque>
#include <set>

namespace triglav::render_core {

JobGraph::JobGraph(graphics_api::Device& device, resource::ResourceManager& resourceManager, PipelineCache& pipelineCache,
                   ResourceStorage& resourceStorage, const Vector2i screenSize) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_pipelineCache(pipelineCache),
    m_resourceStorage(resourceStorage),
    m_screenSize(screenSize)
{
}

BuildContext& JobGraph::add_job(const Name jobName)
{
   auto [buildCtx, ok] = m_contexts.emplace(jobName, BuildContext(m_device, m_resourceManager, m_screenSize));
   assert(ok);

   return buildCtx->second;
}

void JobGraph::add_external_job(Name jobName)
{
   m_externalJobs.emplace_back(jobName);
}

void JobGraph::add_dependency(const Name target, const Name dependency)
{
   m_dependencies.emplace(target, dependency);
}

void JobGraph::add_dependency_to_previous_frame(const Name target, const Name dependency)
{
   m_interframeDependencies.emplace(target, dependency);
}

void JobGraph::enable_flag(const Name job, const Name flag)
{
   m_jobs.at(job).enable_flag(flag);
}

void JobGraph::disable_flag(const Name job, const Name flag)
{
   m_jobs.at(job).disable_flag(flag);
}

void JobGraph::build_jobs(const Name targetJob)
{
   this->deduce_job_order(targetJob);

   for (auto& name : m_jobOrder) {
      if (!m_contexts.contains(name))
         continue;

      auto& ctx = m_contexts.at(name);
      m_jobs.emplace(name, ctx.build_job(m_pipelineCache, m_resourceStorage, name));
      m_jobSemaphores.emplace(name, JobSemaphores{});
   }
   for (const Name jobName : m_externalJobs) {
      m_jobSemaphores.emplace(jobName, JobSemaphores{});
   }
}

graphics_api::Semaphore& JobGraph::semaphore(const Name waitJob, const Name signalJob, const u32 frameIndex)
{
   return m_jobSemaphores.at(waitJob).frameSemaphores[frameIndex].ownedSemaphores.at(signalJob);
}

graphics_api::SemaphoreArrayView JobGraph::wait_semaphores(const Name waitJob, const u32 frameIndex)
{
   return m_jobSemaphores.at(waitJob).frameSemaphores[frameIndex].waitSemaphores;
}

graphics_api::SemaphoreArrayView JobGraph::signal_semaphores(const Name signalJob, const u32 frameIndex)
{
   return m_jobSemaphores.at(signalJob).frameSemaphores[frameIndex].signalSemaphores;
}

void JobGraph::execute(const Name targetJob, const u32 frameIndex, const graphics_api::Fence* fence)
{
   this->build_semaphores();

   this->deduce_job_order(targetJob);

   for (Name jobName : m_jobOrder) {
      if (!m_jobs.contains(jobName)) {
         continue;
      }

      auto& job = m_jobs.at(jobName);
      auto& jobSemaphores = m_jobSemaphores.at(jobName).frameSemaphores[frameIndex];
      const auto* fencePtr = jobName == targetJob ? fence : nullptr;

      auto& waitSems = m_isFirstFrame ? jobSemaphores.waitInFrameSemaphores : jobSemaphores.waitSemaphores;

      job.execute(frameIndex, waitSems, jobSemaphores.signalSemaphores, fencePtr);
   }

   m_isFirstFrame = false;
}

ResourceStorage& JobGraph::resources()
{
   return m_resourceStorage;
}

void JobGraph::build_semaphores()
{
   if (m_hasBuiltSemaphores) {
      return;
   }

   for (auto& [jobName, semaphores] : m_jobSemaphores) {
      auto& jobSemaphores = semaphores.frameSemaphores;

      for (Name dependency : equal_range(m_dependencies, jobName)) {
         for (const u32 frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
            auto [sem, ok] = jobSemaphores[frameIndex].ownedSemaphores.emplace(dependency, GAPI_CHECK(m_device.create_semaphore()));
            assert(ok);

            jobSemaphores[frameIndex].waitSemaphores.add_semaphore(sem->second);
            jobSemaphores[frameIndex].waitInFrameSemaphores.add_semaphore(sem->second);

            auto& depSemaphores = m_jobSemaphores.at(dependency).frameSemaphores[frameIndex];
            depSemaphores.signalSemaphores.add_semaphore(sem->second);
         }
      }

      for (Name dependency : equal_range(m_interframeDependencies, jobName)) {
         for (const u32 frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
            const u32 previousFrameIndex = (frameIndex + FRAMES_IN_FLIGHT_COUNT - 1) % FRAMES_IN_FLIGHT_COUNT;

            auto [sem, ok] = jobSemaphores[frameIndex].ownedSemaphores.emplace(dependency, GAPI_CHECK(m_device.create_semaphore()));
            assert(ok);

            jobSemaphores[frameIndex].waitSemaphores.add_semaphore(sem->second);

            auto& depSemaphores = m_jobSemaphores.at(dependency).frameSemaphores[previousFrameIndex];
            depSemaphores.signalSemaphores.add_semaphore(sem->second);
         }
      }
   }

   m_hasBuiltSemaphores = true;
}

void JobGraph::deduce_job_order(const Name targetJob)
{
   if (m_lastTargetJob.has_value() && *m_lastTargetJob == targetJob) {
      return;
   }

   m_jobOrder.clear();
   m_lastTargetJob = targetJob;

   std::deque<Name> jobQueue;
   jobQueue.emplace_front(targetJob);

   std::set<Name> visited;
   std::set<Name> inserted;

   while (!jobQueue.empty()) {
      Name job = jobQueue.front();

      if (visited.contains(job)) {
         jobQueue.pop_front();

         if (!inserted.contains(job)) {
            m_jobOrder.emplace_back(job);
            inserted.emplace(job);
         }
         continue;
      }

      visited.emplace(job);

      bool allSatisfied = true;
      for (Name dependency : equal_range(m_dependencies, job)) {
         if (!inserted.contains(dependency)) {
            jobQueue.emplace_front(dependency);
            allSatisfied = false;
         }
      }

      if (allSatisfied) {
         jobQueue.pop_front();

         if (!inserted.contains(job)) {
            m_jobOrder.emplace_back(job);
            inserted.emplace(job);
         }
      }
   }
}

}// namespace triglav::render_core