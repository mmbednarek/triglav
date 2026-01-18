#include "Meta.hpp"

#include "TypeRegistry.hpp"

namespace triglav::meta {

PropertyRef::PropertyRef(void* handle, const Member* member) :
    m_handle(handle),
    m_member(member)
{
}

std::string_view PropertyRef::identifier() const
{
   return m_member->identifier;
}

Name PropertyRef::name() const
{
   return m_member->name;
}

Ref PropertyRef::to_ref() const
{
   if (!(m_member->role_flags & MemberRole::Indirect)) {
      return {static_cast<char*>(m_handle) + m_member->property.offset.offset, m_member->property.type_name,
              TypeRegistry::the().type_info(m_member->property.type_name).members};
   }

   if (m_member->role_flags & MemberRole::Reference) {
      return {const_cast<void*>(reinterpret_cast<const void* (*)(void*)>(m_member->property.indirect.get)(m_handle)),
              m_member->property.type_name, TypeRegistry::the().type_info(m_member->property.type_name).members};
   }

   // unsupported
   assert(false);
   std::unreachable();
}

Name PropertyRef::type() const
{
   return m_member->property.type_name;
}

bool PropertyRef::is_array() const
{
   return m_member->role_flags & MemberRole::Array;
}

ArrayRef PropertyRef::to_array_ref() const
{
   return ArrayRef{m_handle, m_member->property.type_name, m_member->property.array};
}

TypeVariant PropertyRef::type_variant() const
{
   if (this->is_array()) {
      return TypeVariant::Array;
   }
   return TypeRegistry::the().type_info(this->type()).variant;
}

ArrayRef::ArrayRef(void* handle, const Name contained_type, const Member::PropertyArray& prop_array) :
    m_handle(handle),
    m_contained_type(contained_type),
    m_prop_array(prop_array)
{
}

Ref ArrayRef::at_ref(const MemorySize index) const
{
   return Ref{m_prop_array.get_n(m_handle, index), m_contained_type, TypeRegistry::the().type_info(m_contained_type).members};
}

Ref ArrayRef::append_ref() const
{
   void* appended_ptr = m_prop_array.append(m_handle);
   return {appended_ptr, m_contained_type, TypeRegistry::the().type_info(m_contained_type).members};
}

Name ArrayRef::type() const
{
   return m_contained_type;
}

TypeVariant ArrayRef::type_variant() const
{
   return TypeRegistry::the().type_info(this->type()).variant;
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

PropertyRef Ref::property_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Property);
   return {m_handle, member};
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