#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/RenderTarget.hpp"
#include "triglav/graphics_api/Synchronization.hpp"

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <utility>

namespace triglav::render_core {

template<typename T>
concept FrameOption = requires(T a) {
   { std::is_same_v<std::underlying_type_t<T>, u32> };
};

class NodeResourcesBase
{
 public:
   virtual ~NodeResourcesBase() = default;

   virtual void add_signal_semaphore(Name child, graphics_api::Semaphore&& semaphore);
   virtual void clean(graphics_api::Device& device);
   virtual void finalize();
   virtual void update_resolution(const graphics_api::Resolution& resolution);
   [[nodiscard]] virtual graphics_api::Semaphore& semaphore(Name child);

 private:
   Heap<Name, graphics_api::Semaphore> m_ownedSemaphores{};
};

class NodeFrameResources : public NodeResourcesBase
{
 public:
   void add_render_target(Name identifier, graphics_api::RenderTarget& renderTarget);
   void add_render_target_with_resolution(Name identifier, graphics_api::RenderTarget& renderTarget,
                                          const graphics_api::Resolution& resolution);
   void update_resolution(const graphics_api::Resolution& resolution) override;

   void register_texture(Name name, graphics_api::Texture& texture);
   graphics_api::Texture& texture(Name name) const;

   [[nodiscard]] graphics_api::Framebuffer& framebuffer(Name identifier);
   [[nodiscard]] graphics_api::CommandList& command_list();
   void add_signal_semaphore(Name child, graphics_api::Semaphore&& semaphore) override;
   void initialize_command_list(graphics_api::SemaphoreArray&& waitSemaphores, graphics_api::CommandList&& commands,
                                size_t inFrameWaitSemaphoreCount);
   graphics_api::SemaphoreArray& wait_semaphores();
   graphics_api::SemaphoreArray& signal_semaphores();
   void finalize() override;

   size_t in_frame_wait_semaphore_count();

 private:
   struct RenderTargetResource
   {
      graphics_api::RenderTarget* renderTarget{};
      std::optional<graphics_api::Framebuffer> framebuffer{};
      std::optional<graphics_api::Resolution> resolution{};
   };

   Heap<Name, RenderTargetResource> m_renderTargets{};
   std::map<Name, graphics_api::Texture*> m_registeredTextures{};
   graphics_api::SemaphoreArray m_signalSemaphores;
   std::optional<graphics_api::SemaphoreArray> m_waitSemaphores{};
   std::optional<graphics_api::CommandList> m_commandList{};
   size_t m_inFrameWaitSemaphoreCount{};
};

class FrameResources
{
 public:
   explicit FrameResources(graphics_api::Device& device);

   template<typename TNode>
   auto& add_node_resources(const Name identifier, TNode&& node)
   {
      auto [it, ok] = m_nodes.emplace(identifier, std::forward<TNode>(node));
      assert(ok);
      return *it->second;
   }

   template<typename TNode = NodeFrameResources>
   TNode& node(const Name identifier)
   {
      return *dynamic_cast<TNode*>(m_nodes.at(identifier).get());
   }

   void add_external_node(Name node);

   [[nodiscard]] graphics_api::Semaphore& target_semaphore(Name targetNode);
   [[nodiscard]] graphics_api::Fence& target_fence();
   [[nodiscard]] graphics_api::Semaphore& semaphore(Name parent, Name child);

   [[nodiscard]] bool has_flag(Name flagName) const;
   void set_flag(Name flagName, bool isEnabled);
   [[nodiscard]] u32 get_option_raw(Name optName) const;
   void set_option_raw(Name optName, u32 option);

   template<FrameOption TOption>
   [[nodiscard]] TOption get_option(const Name optName) const
   {
      return static_cast<TOption>(this->get_option_raw(optName));
   }

   void set_option(const Name optName, const FrameOption auto option)
   {
      this->set_option_raw(optName, static_cast<u32>(option));
   }

   void update_resolution(const graphics_api::Resolution& resolution);
   void add_signal_semaphore(Name parent, Name child, graphics_api::Semaphore&& semaphore);
   void initialize_command_list(Name nodeName, graphics_api::SemaphoreArray&& waitSemaphores, graphics_api::CommandList&& commandList,
                                size_t inFrameWaitSemaphoreCount);
   void clean(graphics_api::Device& device);
   void finalize();

 private:
   std::map<Name, std::unique_ptr<NodeResourcesBase>> m_nodes;
   std::set<Name> m_renderFlags;
   std::map<Name, u32> m_renderOptions;
   graphics_api::Fence m_targetFence;
};

}// namespace triglav::render_core
