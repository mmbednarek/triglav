#pragma once

#include "detail/Commands.hpp"

#include <deque>

namespace triglav::render_core {

class ApplyFlagConditionsPass
{
 public:
   ApplyFlagConditionsPass(std::vector<Name>& flags, u32 enabledFlags);

   void visit(const detail::cmd::IfEnabledCond& cmd);
   void visit(const detail::cmd::IfDisabledCond& cmd);
   void visit(const detail::cmd::EndIfCond& cmd);

   void default_visit(const detail::Command& cmd);

   [[nodiscard]] std::vector<detail::Command>& commands();

 private:
   [[nodiscard]] u32 to_flag_bit(Name flagName) const;

   std::vector<Name>& m_flags;
   u32 m_enabledFlags;
   std::vector<detail::Command> m_commands;
   std::deque<std::tuple<u32, bool>> m_flagStack;
   u32 m_requiredToBeEnabled{0};
   u32 m_requiredToBeDisabled{0};
};

}// namespace triglav::render_core