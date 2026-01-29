#include "Meta.hpp"

#include "TypeRegistry.hpp"

namespace triglav::meta {

using namespace name_literals;

Ref::Ref(void* handle, const Name type_name) :
    m_handle(handle),
    m_type(type_name)
{
}

Name Ref::type() const
{
   return m_type;
}

bool Ref::is_nullptr() const
{
   return m_handle == nullptr;
}

void* Ref::raw_handle() const
{
   return m_handle;
}

TypeVariant Ref::type_variant() const
{
   const auto& type_info = TypeRegistry::the().type_info(m_type);
   return type_info.variant;
}

ClassRef Ref::to_class_ref() const
{
   return {m_handle, m_type};
}

PropertyRef Ref::to_property_ref() const
{
   const auto& info = TypeRegistry::the().type_info(m_type);
   const auto it = std::ranges::find_if(info.members, [](const Member& member) { return member.name == "self"_name; });
   assert(it != info.members.end());
   return {m_handle, &(*it)};
}

EnumRef Ref::to_enum_ref() const
{
   const auto& info = TypeRegistry::the().type_info(m_type);
   const auto it = std::ranges::find_if(info.members, [](const Member& member) { return member.name == "self"_name; });
   assert(it != info.members.end());
   return {m_handle, &(*it)};
}

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

bool PropertyRef::is_lvalue_ref() const
{
   return (m_member->role_flags & MemberRole::Indirect) && !(m_member->role_flags & MemberRole::Reference);
}

Ref PropertyRef::to_ref() const
{
   return {m_handle, m_member->property.type_name};
}

ClassRef PropertyRef::to_class_ref() const
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

EnumRef PropertyRef::to_enum_ref() const
{
   return EnumRef{m_handle, m_member};
}

RefKind PropertyRef::ref_kind() const
{
   if (this->is_array()) {
      return RefKind::Array;
   }
   if (this->is_map()) {
      return RefKind::Map;
   }
   if (this->is_optional()) {
      return RefKind::Optional;
   }

   switch (TypeRegistry::the().type_info(this->type()).variant) {
   case TypeVariant::Primitive:
      return RefKind::Primitive;
   case TypeVariant::Class:
      return RefKind::Class;
   case TypeVariant::Enum:
      return RefKind::Enum;
   }

   std::unreachable();
}

ArrayRef::ArrayRef(void* handle, const Name contained_type, const Member::PropertyArray& prop_array) :
    Ref(handle, contained_type),
    m_prop_array(prop_array)
{
}

Ref ArrayRef::at_ref(const MemorySize index) const
{
   return Ref{m_prop_array.get_n(this->raw_handle(), index), this->type()};
}

Ref ArrayRef::append_ref() const
{
   void* appended_ptr = m_prop_array.append(this->raw_handle());
   return {appended_ptr, this->type()};
}

TypeVariant ArrayRef::type_variant() const
{
   return TypeRegistry::the().type_info(this->type()).variant;
}

EnumRef::EnumRef(void* handle, const Member* member) :
    PropertyRef(handle, member),
    m_members(TypeRegistry::the().type_info(this->type()).members)
{
}

std::string_view EnumRef::string() const
{
   int enum_value = this->value();
   const auto it = std::ranges::find_if(m_members, [enum_value](const Member& member) {
      return member.role_flags & MemberRole::EnumValue && member.enum_value.underlying_value == enum_value;
   });
   if (it == m_members.end()) {
      return {};
   }
   return it->identifier;
}

int EnumRef::value() const
{
   return this->get<int>();
}

void EnumRef::set_string(std::string_view value) const
{
   const auto it = std::ranges::find_if(
      m_members, [value](const Member& member) { return member.role_flags & MemberRole::EnumValue && member.identifier == value; });
   if (it == m_members.end()) {
      return;
   }
   this->set<int>(it->enum_value.underlying_value);
}

MapRef::MapRef(void* handle, const Name value_type, const Member::PropertyMap& prop_array) :
    Ref(handle, value_type),
    m_prop_map(prop_array)
{
}

Name MapRef::key_type() const
{
   return m_prop_map.key_type;
}

Ref MapRef::get_ref(const Ref& key) const
{
   return {m_prop_map.get(this->raw_handle(), key.raw_handle()), this->type()};
}

Ref MapRef::first_key_ref() const
{
   return {const_cast<void*>(m_prop_map.first_key(this->raw_handle())), this->key_type()};
}

Ref MapRef::next_key_ref(const Ref& key) const
{
   return {const_cast<void*>(m_prop_map.next_key(this->raw_handle(), key.raw_handle())), this->key_type()};
}

OptionalRef::OptionalRef(void* handle, const Name contained_type, const Member::PropertyOptional& prop_optional) :
    Ref(handle, contained_type),
    m_prop_optional(prop_optional)
{
}

bool OptionalRef::has_value() const
{
   return m_prop_optional.has_value(this->raw_handle());
}

Ref OptionalRef::get_ref() const
{
   return {m_prop_optional.get(this->raw_handle()), this->type()};
}

void OptionalRef::reset() const
{
   return m_prop_optional.reset(this->raw_handle());
}

std::optional<PropertyRef> PropertyIterator::next()
{
   while (member_index < class_ref->m_members.size() && (!(class_ref->m_members[member_index].role_flags & MemberRole::Property) ||
                                                         class_ref->m_members[member_index].name == "self"_name))
      ++member_index;

   if (member_index >= class_ref->m_members.size()) {
      return std::nullopt;
   }

   return PropertyRef{class_ref->raw_handle(), &class_ref->m_members[member_index++]};
}

ClassRef::ClassRef(void* handle, const Name type, const std::span<Member> members) :
    Ref(handle, type),
    m_members(members)
{
}

ClassRef::ClassRef(void* handle, const Name name) :
    ClassRef(handle, name, TypeRegistry::the().type_info(name).members)
{
}

const Member* ClassRef::find_member(const Name name) const
{
   const auto it = std::ranges::find_if(m_members, [name](const Member& member) { return member.name == name; });
   if (it == m_members.end())
      return nullptr;
   return &(*it);
}

StlForwardRange<PropertyRef, PropertyIterator> ClassRef::properties() const
{
   return StlForwardRange<PropertyRef, PropertyIterator>{this, 0};
}

bool ClassRef::is_array_property(const Name name) const
{
   const auto* member = this->find_member(name);
   if (member == nullptr)
      return false;
   return member->role_flags & MemberRole::Array;
}

PropertyRef ClassRef::property_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Property);
   return {this->raw_handle(), member};
}

ArrayRef ClassRef::property_array_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Array);
   return ArrayRef{static_cast<char*>(this->raw_handle()), member->property.type_name, member->property.array};
}

MapRef ClassRef::property_map_ref(const Name name) const
{
   const auto* member = this->find_member(name);
   assert(member != nullptr && member->role_flags & MemberRole::Map);
   return MapRef{static_cast<char*>(this->raw_handle()), member->property.type_name, member->property.map};
}

int enum_string_to_value(const Name enum_type, const std::string_view str_value)
{
   const auto& type_info = TypeRegistry::the().type_info(enum_type);
   for (const auto& member : type_info.members) {
      if (member.role_flags & MemberRole::EnumValue && member.identifier == str_value) {
         return member.enum_value.underlying_value;
      }
   }

   // fallback
   for (const auto& member : type_info.members) {
      if (member.role_flags & MemberRole::EnumValue && str_value.starts_with(member.identifier)) {
         const auto sep_index = str_value.find('_');
         const auto index = std::stoi(std::string(str_value.substr(sep_index + 1)));
         return member.enum_value.underlying_value + index;
      }
   }

   return -1;
}

}// namespace triglav::meta