#include "TypeRegistry.hpp"

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

#include <array>

namespace triglav::meta {

[[maybe_unused]] void placeholder_func()
{
   // PLACEHOLDER
}

}

#define TG_META_PRIMITIVE(prim_iden, prim_name)                                      \
   std::array TG_CONCAT(TG_META_MEMBERS_, prim_iden){                                \
      ::triglav::meta::Member{                                                       \
         .role_flags = ::triglav::meta::MemberRole::Property,                        \
         .name = ::triglav::make_name_id("self"),                                    \
         .identifier = "self",                                                       \
         .property =                                                                 \
            {                                                                        \
               .type_name = ::triglav::make_name_id(TG_STRING(prim_name)),           \
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


#define TG_META_CLASS_END_NO_CTOR                                                                     \
   }                                                                                                  \
   ;                                                                                                  \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_TYPE(TG_META_JOIN_IDEN)){ \
      {.name = TG_STRING(TG_TYPE(TG_META_JOIN_NS)),                                                   \
       .variant = ::triglav::meta::TypeVariant::Class,                                                \
       .members = TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN)),                            \
       .factory = +[]() -> void* { return new TG_TYPE(TG_META_JOIN_NS)(); }}};

#define TG_TYPE(NS) NS(triglav, Transform3D)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(rotation, triglav::Quaternion)
TG_META_PROPERTY(scale, triglav::Vector3)
TG_META_PROPERTY(translation, triglav::Vector3)
TG_META_CLASS_END_NO_CTOR
#undef TG_TYPE


#undef TG_META_CLASS_END_NO_CTOR
