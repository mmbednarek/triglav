#include "Meta.hpp"

#include "TypeRegistry.hpp"

namespace triglav::meta {

Ref::Ref(void* handle, const Name type, const std::span<ClassMember> members) :
    m_handle(handle),
    m_type(type),
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

Ref Ref::property_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->type == ClassMemberType::Property);
   const auto& type_info = TypeRegistry::the().type_info(member->property.type_name);
   return {static_cast<char*>(m_handle) + member->property.offset, member->property.type_name, type_info.members};
}

Name Ref::type() const
{
   return m_type;
}

}// namespace triglav::meta