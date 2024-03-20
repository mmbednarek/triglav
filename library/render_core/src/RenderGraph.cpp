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

   std::map<NameID, NodeState> visited{};
   std::stack<NameID> nodes;
   nodes.emplace(targetNode);

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
            auto &bakedNode = m_bakedNodes[m_nodeIndicies.at(it->second)];
            bakedNode.signalSemaphorePtrs.emplace_back(semaphore);
            bakedNode.signalSemaphores.add_semaphore(*semaphore);
            semaphores.add_semaphore(*semaphore);
         }

         ++it;
      }

      auto &node       = m_nodes[currentNode];
      auto commandList = GAPI_CHECK(m_device.queue_manager().create_command_list(node->work_types()));

      m_nodeIndicies.emplace(currentNode, m_bakedNodes.size());
      m_bakedNodes.emplace_back(
              BakedNode{currentNode, std::move(semaphores), {}, std::move(commandList), {}});
   }

   auto *semaphore = m_device.queue_manager().aquire_semaphore();
   m_bakedNodes.back().signalSemaphorePtrs.emplace_back(semaphore);
   m_bakedNodes.back().signalSemaphores.add_semaphore(*semaphore);

   return true;
}

void RenderGraph::record_command_lists()
{
   for (auto &bakedNode : m_bakedNodes) {
      const auto &node = m_nodes[bakedNode.name];
      GAPI_CHECK_STATUS(bakedNode.commandList.reset());
      GAPI_CHECK_STATUS(bakedNode.commandList.begin());
      node->record_commands(bakedNode.commandList);
      GAPI_CHECK_STATUS(bakedNode.commandList.finish());
   }
}

graphics_api::Status RenderGraph::execute()
{
   for (const auto &bakedNode : m_bakedNodes) {
      const graphics_api::Fence *fence{};
      if (bakedNode.name == m_bakedNodes.back().name) {
         fence = &m_targetFence;
      }

      const auto status = m_device.submit_command_list(bakedNode.commandList, bakedNode.waitSemaphores,
                                                       bakedNode.signalSemaphores, fence);
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
   return m_bakedNodes.back().signalSemaphorePtrs[0];
}

u32 RenderGraph::triangle_count(const NameID node) const
{
    return m_bakedNodes[m_nodeIndicies.at(node)].commandList.triangle_count();
}

void RenderGraph::clean()
{
   for (const auto &node : m_bakedNodes) {
      for (const auto *semaphorePtr : node.signalSemaphorePtrs) {
         m_device.queue_manager().release_semaphore(semaphorePtr);
      }
   }
   m_bakedNodes.clear();
   m_nodeIndicies.clear();
}

}// namespace triglav::render_core
