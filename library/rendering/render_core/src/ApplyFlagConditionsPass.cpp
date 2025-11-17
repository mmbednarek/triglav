#include "ApplyFlagConditionsPass.hpp"

namespace triglav::render_core {

ApplyFlagConditionsPass::ApplyFlagConditionsPass(std::vector<Name>& flags, const u32 enabled_flags) :
    m_flags(flags),
    m_enabled_flags(enabled_flags)
{
}

void ApplyFlagConditionsPass::visit(const detail::cmd::IfEnabledCond& cmd)
{
   const auto flag_bit = this->to_flag_bit(cmd.flag);
   m_flag_stack.emplace_front(flag_bit, true);
   m_required_to_be_enabled |= flag_bit;
}

void ApplyFlagConditionsPass::visit(const detail::cmd::IfDisabledCond& cmd)
{
   const auto flag_bit = this->to_flag_bit(cmd.flag);
   m_flag_stack.emplace_front(flag_bit, false);
   m_required_to_be_disabled |= flag_bit;
}

void ApplyFlagConditionsPass::visit(const detail::cmd::EndIfCond& /*cmd*/)
{
   const auto [flag, is_enabled] = m_flag_stack.front();
   m_flag_stack.pop_front();
   if (is_enabled) {
      m_required_to_be_enabled &= ~flag;
   } else {
      m_required_to_be_disabled &= ~flag;
   }
}

void ApplyFlagConditionsPass::default_visit(const detail::Command& cmd)
{
   if ((m_required_to_be_enabled == 0 || (m_enabled_flags & m_required_to_be_enabled) != 0) &&
       (m_required_to_be_disabled == 0 || (m_enabled_flags & m_required_to_be_disabled) == 0)) {
      m_commands.push_back(cmd);
   }
}

std::vector<detail::Command>& ApplyFlagConditionsPass::commands()
{
   return m_commands;
}

u32 ApplyFlagConditionsPass::to_flag_bit(const Name flag_name) const
{
   return 1u << (std::ranges::find(m_flags, flag_name) - m_flags.begin());
}

}// namespace triglav::render_core
