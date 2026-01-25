#include "TypeRegistry.hpp"

namespace triglav::meta {

using namespace name_literals;

#define TG_META_PRIMITIVE(prim_iden, prim_name)                                      \
   std::array TG_CONCAT(TG_META_MEMBERS_, prim_iden){                                \
      Member{                                                                        \
         .role_flags = MemberRole::Property,                                         \
         .name = "self"_name,                                                        \
         .identifier = "self",                                                       \
         .property =                                                                 \
            {                                                                        \
               .type_name = make_name_id(TG_STRING(prim_name)),                      \
               .offset =                                                             \
                  {                                                                  \
                     .offset = 0,                                                    \
                  },                                                                 \
            },                                                                       \
      },                                                                             \
   };                                                                                \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, prim_iden){ \
      {                                                                              \
         .name = TG_STRING(prim_name),                                               \
         .variant = ::triglav::meta::TypeVariant::Primitive,                         \
         .members = TG_CONCAT(TG_META_MEMBERS_, prim_iden),                          \
         .factory = +[]() -> void* { return new prim_name(); },                      \
      },                                                                             \
   };

TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE

void TypeRegistry::register_type(Type tp)
{
   m_types.emplace(make_name_id(tp.name), std::move(tp));
}

Box TypeRegistry::create_box(const Name type) const
{
   const auto& ty = m_types.at(type);
   return {ty.factory(), type, ty.members};
}

TypeRegistry& TypeRegistry::the()
{
   static TypeRegistry registry;
   return registry;
}

const Type& TypeRegistry::type_info(const Name type_name) const
{
   return m_types.at(type_name);
}

}// namespace triglav::meta
