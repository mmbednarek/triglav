#include "ApplyFlagConditionsPass.hpp"

namespace triglav::render_core {

ApplyFlagConditionsPass::ApplyFlagConditionsPass(std::vector<Name>& flags, const u32 enabledFlags) :
    m_flags(flags),
    m_enabledFlags(enabledFlags)
{
}

void ApplyFlagConditionsPass::visit(const detail::cmd::IfEnabledCond& cmd)
{
   const auto flagBit = this->to_flag_bit(cmd.flag);
   m_flagStack.emplace_front(flagBit, true);
   m_requiredToBeEnabled |= flagBit;
}

void ApplyFlagConditionsPass::visit(const detail::cmd::IfDisabledCond& cmd)
{
   const auto flagBit = this->to_flag_bit(cmd.flag);
   m_flagStack.emplace_front(flagBit, false);
   m_requiredToBeDisabled |= flagBit;
}

void ApplyFlagConditionsPass::visit(const detail::cmd::EndIfCond& /*cmd*/)
{
   const auto [flag, isEnabled] = m_flagStack.front();
   m_flagStack.pop_front();
   if (isEnabled) {
      m_requiredToBeEnabled &= ~flag;
   } else {
      m_requiredToBeDisabled &= ~flag;
   }
}

void ApplyFlagConditionsPass::default_visit(const detail::Command& cmd)
{
   if ((m_requiredToBeEnabled == 0 || (m_enabledFlags & m_requiredToBeEnabled) != 0) &&
       (m_requiredToBeDisabled == 0 || (m_enabledFlags & m_requiredToBeDisabled) == 0)) {
      m_commands.push_back(cmd);
   }
}

std::vector<detail::Command>& ApplyFlagConditionsPass::commands()
{
   return m_commands;
}

u32 ApplyFlagConditionsPass::to_flag_bit(const Name flagName) const
{
   return 1u << (std::ranges::find(m_flags, flagName) - m_flags.begin());
}

}// namespace triglav::render_core
