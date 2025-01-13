#pragma once

#include "detail/Commands.hpp"

#include "triglav/Name.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

namespace triglav::render_core {

class BuildContext;

class BarrierInsertionPass
{
 public:
   explicit BarrierInsertionPass(BuildContext& context);

   void visit(const detail::cmd::BindDescriptors& cmd);
   void visit(const detail::cmd::BindVertexBuffer& cmd);
   void visit(const detail::cmd::BindIndexBuffer& cmd);
   void visit(const detail::cmd::CopyTextureToBuffer& cmd);
   void visit(const detail::cmd::CopyBufferToTexture& cmd);
   void visit(const detail::cmd::CopyBuffer& cmd);
   void visit(const detail::cmd::FillBuffer& cmd);
   void visit(const detail::cmd::BeginRenderPass& cmd);
   void visit(const detail::cmd::EndRenderPass& cmd);
   void visit(const detail::cmd::DrawIndirectWithCount& cmd);
   void visit(const detail::cmd::ExportTexture& cmd);
   void visit(const detail::cmd::ExportBuffer& cmd);

   void default_visit(const detail::Command& cmd);

   [[nodiscard]] std::vector<detail::Command>& commands();

 private:
   template<typename TCmd, typename... TArgs>
   TCmd& add_command(TArgs&&... args)
   {
      m_commands.emplace_back(std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...);
      return std::get<TCmd>(m_commands.back());
   }

   template<typename TCmd, typename... TArgs>
   TCmd& add_command_before_render_pass(TArgs&&... args)
   {
      if (!m_isWithinRenderPass) {
         return this->add_command<TCmd>(std::forward<TArgs>(args)...);
      }

      auto it = std::find_if(m_commands.rbegin(), m_commands.rend(),
                             [](const auto& cmd) { return std::holds_alternative<detail::cmd::BeginRenderPass>(cmd); });
      return std::get<TCmd>(*m_commands.emplace(it.base() - 1, std::in_place_type_t<TCmd>{}, std::forward<TArgs>(args)...));
   }

   void setup_texture_barrier(TextureRef texRef, graphics_api::TextureState targetState, graphics_api::PipelineStage targetStage,
                              std::optional<graphics_api::PipelineStage> lastUsedStage = std::nullopt);

   void setup_buffer_barrier(BufferRef buffRef, graphics_api::BufferAccess targetAccess, graphics_api::PipelineStage targetStage);

   BuildContext& m_context;
   std::vector<detail::Command> m_commands;
   bool m_isWithinRenderPass{false};
};

}// namespace triglav::render_core
