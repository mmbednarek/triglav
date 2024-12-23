#include "RenderGraph.hpp"

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Synchronization.hpp"

#include <stack>

namespace triglav::render_core {

using namespace name_literals;

RenderGraph::RenderGraph(graphics_api::Device& device) :
    m_device(device),
    m_frameResources{FrameResources{device}, FrameResources{device}, FrameResources{device}}
{
}

void RenderGraph::add_external_node(Name node)
{
   m_externalNodes.emplace(node);
}

void RenderGraph::add_dependency(Name target, Name dependency)
{
   m_dependencies.emplace(target, dependency);
}

void RenderGraph::add_interframe_dependency(Name target, Name dependency)
{
   m_interframeDependencies.emplace(target, dependency);
}

bool RenderGraph::bake(Name targetNode)
{
   enum class NodeState
   {
      Unvisited,
      Visited,
      Baked,
   };

   this->clean();
   this->initialize_nodes();

   std::map<Name, NodeState> visited{};
   std::stack<Name> nodes;
   nodes.emplace(targetNode);

   m_targetNode = targetNode;

   // iterate BFS through nodes

   while (not nodes.empty()) {
      auto currentNode = nodes.top();

      if (m_externalNodes.contains(currentNode)) {
         nodes.pop();
         continue;
      }

      if (visited[currentNode] == NodeState::Baked) {
         nodes.pop();
         continue;
      }

      if (visited[currentNode] == NodeState::Unvisited) {
         visited[currentNode] = NodeState::Visited;

         auto [it, end] = m_dependencies.equal_range(currentNode);
         if (it != end) {
            while (it != end) {
               nodes.emplace(it->second);
               ++it;
            }

            continue;
         }
      }

      visited[currentNode] = NodeState::Baked;

      nodes.pop();

      for (u32 i = 0; i < m_frameResources.size(); ++i) {
         auto& frameRes = m_frameResources[i];
         auto& previousFrameRes = m_frameResources[i == 0 ? (m_frameResources.size() - 1) : (i - 1)];
         graphics_api::SemaphoreArray semaphores{};

         // In frame dependencies
         auto [it, end] = m_dependencies.equal_range(currentNode);
         while (it != end) {
            auto semaphore = GAPI_CHECK(m_device.create_semaphore());
            semaphores.add_semaphore(semaphore);
            frameRes.add_signal_semaphore(it->second, currentNode, std::move(semaphore));

            ++it;
         }

         const auto inFrameSemaphoreCount = semaphores.semaphore_count();

         // Dependencies between current end previous frame
         std::tie(it, end) = m_interframeDependencies.equal_range(currentNode);
         while (it != end) {
            auto semaphore = GAPI_CHECK(m_device.create_semaphore());
            semaphores.add_semaphore(semaphore);
            previousFrameRes.add_signal_semaphore(it->second, currentNode, std::move(semaphore));
            ++it;
         }

         auto& node = m_nodes[currentNode];
         auto commandList = GAPI_CHECK(m_device.queue_manager().create_command_list(node->work_types()));

         frameRes.initialize_command_list(currentNode, std::move(semaphores), std::move(commandList), inFrameSemaphoreCount);
      }

      m_nodeOrder.emplace_back(currentNode);
   }

   for (auto& frameRes : m_frameResources) {
      frameRes.add_signal_semaphore(targetNode, "__TARGET__"_name, GAPI_CHECK(m_device.create_semaphore()));
      frameRes.finalize();
   }

   return true;
}

void RenderGraph::initialize_nodes()
{
   for (const auto name : m_externalNodes) {
      for (auto& frameRes : m_frameResources) {
         frameRes.add_external_node(name);
      }
   }

   for (const auto& [name, node] : m_nodes) {
      for (auto& frameRes : m_frameResources) {
         frameRes.add_node_resources(name, node->create_node_resources());
      }
   }
}

void RenderGraph::record_command_lists()
{
   for (const auto& [name, node] : m_nodes) {
      auto& resources = this->active_frame_resources().node<NodeFrameResources>(name);
      auto& cmdList = resources.command_list();
      GAPI_CHECK_STATUS(cmdList.reset());
      GAPI_CHECK_STATUS(cmdList.begin());
      node->record_commands(this->active_frame_resources(), resources, cmdList);
      GAPI_CHECK_STATUS(cmdList.finish());
   }
}

void RenderGraph::update_resolution(const graphics_api::Resolution& resolution)
{
   for (auto& frameRes : m_frameResources) {
      frameRes.update_resolution(resolution);
   }
}

graphics_api::Status RenderGraph::execute()
{
   for (const auto& name : m_nodeOrder) {
      auto& resources = this->active_frame_resources().node<NodeFrameResources>(name);

      const graphics_api::Fence* fence{};
      if (name == m_targetNode) {
         fence = &this->active_frame_resources().target_fence();
      }
      graphics_api::SemaphoreArrayView waitSemaphores{};
      if (m_firstFrame) {
         waitSemaphores = {resources.wait_semaphores(), resources.in_frame_wait_semaphore_count()};
      } else {
         waitSemaphores = resources.wait_semaphores();
      }

      const auto status = m_device.submit_command_list(resources.command_list(), waitSemaphores, resources.signal_semaphores(), fence,
                                                       this->node<IRenderNode>(name).work_types());
      if (status != graphics_api::Status::Success) {
         return status;
      }
   }

   m_firstFrame = false;

   return graphics_api::Status::Success;
}

void RenderGraph::await()
{
   this->active_frame_resources().target_fence().await();
}

graphics_api::Semaphore& RenderGraph::target_semaphore()
{
   return this->active_frame_resources().target_semaphore(m_targetNode);
}

u32 RenderGraph::triangle_count(const Name node)
{
   auto& resources = this->active_frame_resources().node(node);
   return static_cast<u32>(resources.command_list().triangle_count());
}

void RenderGraph::clean()
{
   for (auto& frameRes : m_frameResources) {
      frameRes.clean(m_device);
   }
}

void RenderGraph::set_flag(const Name flag, const bool isEnabled)
{
   for (auto& frameRes : m_frameResources) {
      frameRes.set_flag(flag, isEnabled);
   }
}

FrameResources& RenderGraph::active_frame_resources()
{
   return m_frameResources[m_activeFrame];
}

FrameResources& RenderGraph::previous_frame_resources()
{
   return m_frameResources[m_previousFrame];
}

void RenderGraph::change_active_frame()
{
   m_previousFrame = std::exchange(m_activeFrame, (m_activeFrame + 1) % m_frameResources.size());
}

graphics_api::Semaphore& RenderGraph::semaphore(Name parent, Name child)
{
   return this->active_frame_resources().semaphore(parent, child);
}

}// namespace triglav::render_core
