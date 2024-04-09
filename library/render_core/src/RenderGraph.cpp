#include "RenderGraph.h"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Synchronization.h"

#include <ranges>
#include <stack>

namespace triglav::render_core {

RenderGraph::RenderGraph(graphics_api::Device &device) :
    m_targetFence(GAPI_CHECK(device.create_fence())),
    m_device(device)
{
}

void RenderGraph::add_semaphore_node(NameID node, graphics_api::Semaphore *semaphore)
{
   m_semaphoreNodes.emplace(node, semaphore);
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

      if (m_semaphoreNodes.contains(currentNode)) {
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

      graphics_api::SemaphoreArray semaphores{};
      auto [it, end] = m_dependencies.equal_range(currentNode);
      while (it != end) {
         if (m_semaphoreNodes.contains(it->second)) {
            semaphores.add_semaphore(*m_semaphoreNodes[it->second]);
         } else {
            auto *semaphore = m_device.queue_manager().aquire_semaphore();
            m_frameResources.add_signal_semaphore(it->second, semaphore);
            semaphores.add_semaphore(*semaphore);
         }

         ++it;
      }

      auto &node       = m_nodes[currentNode];
      auto commandList = GAPI_CHECK(m_device.queue_manager().create_command_list(node->work_types()));

      m_frameResources.initialize_command_list(currentNode, std::move(semaphores), std::move(commandList));
      m_nodeOrder.emplace_back(currentNode);
   }

   m_targetSemaphore = m_device.queue_manager().aquire_semaphore();
   m_frameResources.add_signal_semaphore(targetNode, m_targetSemaphore);
   return true;
}

void RenderGraph::initialize_nodes()
{
   for (const auto &[name, node] : m_nodes) {
      m_frameResources.add_node_resources(name, node->create_node_resources());
   }
}

void RenderGraph::record_command_lists()
{
   for (const auto &[name, node] : m_nodes) {
      auto &resources = m_frameResources.node<NodeFrameResources>(name);
      auto &cmdList   = resources.command_list();
      GAPI_CHECK_STATUS(cmdList.reset());
      GAPI_CHECK_STATUS(cmdList.begin());
      node->record_commands(m_frameResources, resources, cmdList);
      GAPI_CHECK_STATUS(cmdList.finish());
   }
}

void RenderGraph::reset_command_lists()
{
   for (const auto &name : m_nodes | std::views::keys) {
      auto &resources = m_frameResources.node<NodeFrameResources>(name);
      GAPI_CHECK_STATUS(resources.command_list().reset());
   }
}

void RenderGraph::update_resolution(const graphics_api::Resolution &resolution)
{
   m_frameResources.update_resolution(resolution);
}

graphics_api::Status RenderGraph::execute()
{
   for (const auto &name : m_nodeOrder) {
      auto &resources = m_frameResources.node<NodeFrameResources>(name);

      const graphics_api::Fence *fence{};
      if (name == m_targetNode) {
         fence = &m_targetFence;
      }

      const auto status = m_device.submit_command_list(resources.command_list(), resources.wait_semaphores(),
                                                       resources.signal_semaphores(), fence);
      if (status != graphics_api::Status::Success) {
         return status;
      }
   }
   return graphics_api::Status::Success;
}

void RenderGraph::await() const
{
   m_targetFence.await();
}

graphics_api::Semaphore *RenderGraph::target_semaphore() const
{
   return m_targetSemaphore;
}

u32 RenderGraph::triangle_count(const NameID node)
{
   auto &resources = m_frameResources.node<NodeFrameResources>(node);
   return static_cast<u32>(resources.command_list().triangle_count());
}

void RenderGraph::clean()
{
   m_frameResources.clean(m_device);
}

void RenderGraph::set_flag(NameID flag, const bool isEnabled)
{
   m_frameResources.set_flag(flag, isEnabled);
}

}// namespace triglav::render_core
