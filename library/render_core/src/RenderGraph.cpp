#include "RenderGraph.h"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Synchronization.h"

#include <ranges>
#include <stack>

namespace triglav::render_core {

using namespace name_literals;

RenderGraph::RenderGraph(graphics_api::Device &device) :
    m_device(device),
    m_frameResources{FrameResources{device}, FrameResources{device}, FrameResources{device}}
{
}

void RenderGraph::add_external_node(NameID node)
{
   m_externalNodes.emplace(node);
}

void RenderGraph::add_dependency(NameID target, NameID dependency)
{
   m_dependencies.emplace(target, dependency);
}

bool RenderGraph::bake(NameID targetNode)
{
   enum class NodeState
   {
      Unvisited,
      Visited,
      Baked,
   };

   this->clean();
   this->initialize_nodes();

   std::map<NameID, NodeState> visited{};
   std::stack<NameID> nodes;
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

      for (auto& frameRes : m_frameResources) {
         graphics_api::SemaphoreArray semaphores{};
         auto [it, end] = m_dependencies.equal_range(currentNode);
         while (it != end) {
            auto semaphore = GAPI_CHECK(m_device.create_semaphore());
            semaphores.add_semaphore(semaphore);
            frameRes.add_signal_semaphore(it->second, currentNode, std::move(semaphore));

            ++it;
         }

         auto &node       = m_nodes[currentNode];
         auto commandList = GAPI_CHECK(m_device.queue_manager().create_command_list(node->work_types()));

         frameRes.initialize_command_list(currentNode, std::move(semaphores), std::move(commandList));
      }

      m_nodeOrder.emplace_back(currentNode);
   }

   for (auto& frameRes : m_frameResources) {
      frameRes.add_signal_semaphore(targetNode, "__TARGET__"_name_id, GAPI_CHECK(m_device.create_semaphore()));
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

   for (const auto &[name, node] : m_nodes) {
      for (auto& frameRes : m_frameResources) {
         frameRes.add_node_resources(name, node->create_node_resources());
      }
   }
}

void RenderGraph::record_command_lists()
{
   for (const auto &[name, node] : m_nodes) {
      auto &resources = this->active_frame_resources().node<NodeFrameResources>(name);
      auto &cmdList   = resources.command_list();
      GAPI_CHECK_STATUS(cmdList.reset());
      GAPI_CHECK_STATUS(cmdList.begin());
      node->record_commands(this->active_frame_resources(), resources, cmdList);
      GAPI_CHECK_STATUS(cmdList.finish());
   }
}

void RenderGraph::update_resolution(const graphics_api::Resolution &resolution)
{
   for (auto& frameRes : m_frameResources) {
      frameRes.update_resolution(resolution);
   }
}

graphics_api::Status RenderGraph::execute()
{
   for (const auto &name : m_nodeOrder) {
      auto &resources = this->active_frame_resources().node<NodeFrameResources>(name);

      const graphics_api::Fence *fence{};
      if (name == m_targetNode) {
         fence = &this->active_frame_resources().target_fence();
      }

      const auto status = m_device.submit_command_list(resources.command_list(), resources.wait_semaphores(),
                                                       resources.signal_semaphores(), fence);
      if (status != graphics_api::Status::Success) {
         return status;
      }
   }
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

u32 RenderGraph::triangle_count(const NameID node)
{
   auto &resources = this->active_frame_resources().node(node);
   return static_cast<u32>(resources.command_list().triangle_count());
}

void RenderGraph::clean()
{
   for (auto& frameRes : m_frameResources) {
      frameRes.clean(m_device);
   }
}

void RenderGraph::set_flag(NameID flag, const bool isEnabled)
{
   for (auto& frameRes : m_frameResources) {
      frameRes.set_flag(flag, isEnabled);
   }
}

FrameResources &RenderGraph::active_frame_resources()
{
   return m_frameResources[m_activeFrame];
}

void RenderGraph::swap_frames()
{
   m_activeFrame = (m_activeFrame + 1) % m_frameResources.size();
}

graphics_api::Semaphore& RenderGraph::semaphore(NameID parent, NameID child)
{
   return this->active_frame_resources().semaphore(parent, child);
}

}// namespace triglav::render_core
