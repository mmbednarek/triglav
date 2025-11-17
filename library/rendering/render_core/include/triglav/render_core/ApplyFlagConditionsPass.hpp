#pragma once

#include "detail/Commands.hpp"

#include <deque>

namespace triglav::render_core {

class ApplyFlagConditionsPass
{
 public:
   ApplyFlagConditionsPass(std::vector<Name>& flags, u32 enabled_flags);

   void visit(const detail::cmd::IfEnabledCond& cmd);
   void visit(const detail::cmd::IfDisabledCond& cmd);
   void visit(const detail::cmd::EndIfCond& cmd);

   void default_visit(const detail::Command& cmd);

   [[nodiscard]] std::vector<detail::Command>& commands();

 private:
   [[nodiscard]] u32 to_flag_bit(Name flag_name) const;

   std::vector<Name>& m_flags;
   u32 m_enabled_flags;
   std::vector<detail::Command> m_commands;
   std::deque<std::tuple<u32, bool>> m_flag_stack;
   u32 m_required_to_be_enabled{0};
   u32 m_required_to_be_disabled{0};
};

}// namespace triglav::render_core