#include "Meta.hpp"

#include "TypeRegistry.hpp"

namespace triglav::meta {

ArrayRef::ArrayRef(void* handle, const Name contained_type, const Member::PropertyArray& prop_array) :
    m_handle(handle),
    m_contained_type(contained_type),
    m_prop_array(prop_array)
{
}

Ref ArrayRef::append_ref()
{
   void* appended_ptr = m_prop_array.append(m_handle);
   return {appended_ptr, m_contained_type, TypeRegistry::the().type_info(m_contained_type).members};
}

Ref::Ref(void* handle, const Name type, const std::span<Member> members) :
    m_handle(handle),
    m_type(type),
    m_members(members)
{
}

const Member* Ref::find_member(const Name name) const
{
   const auto it = std::ranges::find_if(m_members, [name](const Member& member) { return member.name == name; });
   if (it == m_members.end())
      return nullptr;
   return &(*it);
}

bool Ref::is_array_property(const Name name) const
{
   const auto* member = this->find_member(name);
   if (member == nullptr)
      return false;
   return member->role_flags & MemberRole::Array;
}

Ref Ref::property_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Property);
   if (member->role_flags & MemberRole::Indirect && member->role_flags & MemberRole::Reference) {
      const auto& type_info = TypeRegistry::the().type_info(member->property.type_name);
      return {const_cast<void*>(reinterpret_cast<const void* (*)(void*)>(member->property.indirect.get)(m_handle)),
              member->property.type_name, type_info.members};
   }
   if (member->role_flags & MemberRole::Property) {
      const auto& type_info = TypeRegistry::the().type_info(member->property.type_name);
      return {static_cast<char*>(m_handle) + member->property.offset.offset, member->property.type_name, type_info.members};
   }
   return {nullptr, 0, {}};
}

ArrayRef Ref::property_array_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Array);
   return ArrayRef{static_cast<char*>(m_handle), member->property.type_name, member->property.array};
}

Name Ref::type() const
{
   return m_type;
}

}// namespace triglav::meta