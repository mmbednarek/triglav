#include "TypeRegistry.hpp"

namespace triglav::meta {

using namespace name_literals;

#define TG_REGISTER_PRIMITIVE(prim_iden, prim_name)                                                                             \
   std::array TG_CONCAT(TG_META_MEMBERS_, prim_iden){                                                                           \
      ClassMember{                                                                                                              \
         .type = ClassMemberType::Property,                                                                                     \
         .name = "get"_name,                                                                                                    \
         .identifier = "get",                                                                                                   \
         .function =                                                                                                            \
            {                                                                                                                   \
               .pointer = reinterpret_cast<void*>(+[](void* handle) -> prim_name { return *static_cast<prim_name*>(handle); }), \
            },                                                                                                                  \
      },                                                                                                                        \
   };                                                                                                                           \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, prim_iden){                                            \
      {                                                                                                                         \
         .name = TG_STRING(prim_name),                                                                                          \
         .variant = ::triglav::meta::TypeVariant::Primitive,                                                                    \
         .members = TG_CONCAT(TG_META_MEMBERS_, prim_iden),                                                                     \
         .factory = +[]() -> void* { return new prim_name(); },                                                                 \
      },                                                                                                                        \
   };

TG_REGISTER_PRIMITIVE(char, char)
TG_REGISTER_PRIMITIVE(int, int)
TG_REGISTER_PRIMITIVE(i8, i8)
TG_REGISTER_PRIMITIVE(u8, u8)
TG_REGISTER_PRIMITIVE(i16, i16)
TG_REGISTER_PRIMITIVE(u16, u16)
TG_REGISTER_PRIMITIVE(i32, i32)
TG_REGISTER_PRIMITIVE(u32, u32)
TG_REGISTER_PRIMITIVE(i64, i64)
TG_REGISTER_PRIMITIVE(u64, u64)
TG_REGISTER_PRIMITIVE(float, float)
TG_REGISTER_PRIMITIVE(double, double)
TG_REGISTER_PRIMITIVE(std__string, std::string)
TG_REGISTER_PRIMITIVE(std__string_view, std::string_view)

void TypeRegistry::register_type(Type tp)
{
   m_types.emplace(make_name_id(tp.name), std::move(tp));
}

Box TypeRegistry::create_box(const Name type) const
{
   const auto& ty = m_types.at(type);
   return {ty.factory(), ty.members};
}

TypeRegistry& TypeRegistry::the()
{
   static TypeRegistry registry;
   return registry;
}

}// namespace triglav::meta
