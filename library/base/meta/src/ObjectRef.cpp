#include "Meta.hpp"

namespace triglav::meta {

Ref::Ref(void* handle, std::span<ClassMember> members) :
    m_handle(handle),
    m_members(members)
{
}

const ClassMember* Ref::find_member(const Name name) const
{
   const auto it = std::ranges::find_if(m_members, [name](const ClassMember& member) { return member.name == name; });
   if (it == m_members.end())
      return nullptr;
   return &(*it);
}

}// namespace triglav::meta