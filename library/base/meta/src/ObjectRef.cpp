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

bool PropertyRef::is_map() const
{
   return m_member->role_flags & MemberRole::Map;
}

bool PropertyRef::is_optional() const
{
   return m_member->role_flags & MemberRole::Optional;
}

ArrayRef PropertyRef::to_array_ref() const
{
   return ArrayRef{m_handle, m_member->property.type_name, m_member->property.array};
}

MapRef PropertyRef::to_map_ref() const
{
   return MapRef{m_handle, m_member->property.type_name, m_member->property.map};
}

OptionalRef PropertyRef::to_optional_ref() const
{
   return OptionalRef{m_handle, m_member->property.type_name, m_member->property.optional};
}

TypeVariant PropertyRef::type_variant() const
{
   if (this->is_array()) {
      return TypeVariant::Array;
   }
   if (this->is_map()) {
      return TypeVariant::Map;
   }
   if (this->is_optional()) {
      return TypeVariant::Optional;
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

MapRef::MapRef(void* handle, const Name value_type, const Member::PropertyMap& prop_array) :
    m_handle(handle),
    m_value_type(value_type),
    m_prop_map(prop_array)
{
}

Name MapRef::key_type() const
{
   return m_prop_map.key_type;
}

Name MapRef::value_type() const
{
   return m_value_type;
}

Ref MapRef::get_ref(const Ref& key) const
{
   return {m_prop_map.get_n(m_handle, key.raw_handle()), this->value_type()};
}

Ref MapRef::first_key_ref() const
{
   return {const_cast<void*>(m_prop_map.first_key(m_handle)), this->key_type()};
}

Ref MapRef::next_key_ref(const Ref& key) const
{
   return {const_cast<void*>(m_prop_map.next_key(m_handle, key.raw_handle())), this->key_type()};
}

OptionalRef::OptionalRef(void* handle, const Name contained_type, const Member::PropertyOptional& prop_optional) :
    m_handle(handle),
    m_contained_type(contained_type),
    m_prop_optional(prop_optional)
{
}

bool OptionalRef::has_value() const
{
   return m_prop_optional.has_value(m_handle);
}

Ref OptionalRef::get_ref() const
{
   return {m_prop_optional.get(m_handle), m_contained_type};
}

Ref::Ref(void* handle, const Name type, const std::span<Member> members) :
    m_handle(handle),
    m_type(type),
    m_members(members)
{
}

Ref::Ref(void* handle, const Name name) :
    m_handle(handle),
    m_type(name),
    m_members(TypeRegistry::the().type_info(name).members)
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

MapRef Ref::property_map_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Map);
   return MapRef{static_cast<char*>(m_handle), member->property.type_name, member->property.map};
}

bool Ref::is_nullptr() const
{
   return m_handle == nullptr;
}

void* Ref::raw_handle() const
{
   return m_handle;
}

Name Ref::type() const
{
   return m_type;
}

}// namespace triglav::meta