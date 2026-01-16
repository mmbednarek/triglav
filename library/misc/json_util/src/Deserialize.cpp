#include "Deserialize.hpp"

#include "JsonUtil.hpp"

#include "triglav/meta/TypeRegistry.hpp"

namespace triglav::json_util {

using namespace name_literals;

#define TG_JSON_GETTER_char val.GetInt()
#define TG_JSON_GETTER_int val.GetInt()
#define TG_JSON_GETTER_i8 val.GetInt()
#define TG_JSON_GETTER_u8 val.GetUint()
#define TG_JSON_GETTER_i16 val.GetInt()
#define TG_JSON_GETTER_u16 val.GetUint()
#define TG_JSON_GETTER_i32 val.GetInt()
#define TG_JSON_GETTER_u32 val.GetUint()
#define TG_JSON_GETTER_i64 val.GetInt64()
#define TG_JSON_GETTER_u64 val.GetUint64()
#define TG_JSON_GETTER_float val.GetFloat()
#define TG_JSON_GETTER_double val.GetDouble()
#define TG_JSON_GETTER_std__string val.GetString()
#define TG_JSON_GETTER_std__string_view \
   std::string_view {}// shouldn't use string views for deserialization
#define TG_JSON_GETTER(x) TG_CONCAT(TG_JSON_GETTER_, x)

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src);

void deserialize_array_class(meta::ArrayRef& dst, const rapidjson::Value& src)
{
   const auto& src_arr = src.GetArray();
   for ([[maybe_unused]] const auto& val : src_arr) {
      auto ref = dst.append_ref();
      deserialize_value(ref, val);
   }
}

void deserialize_array(meta::ArrayRef& dst, const Name type_name, const rapidjson::Value& src)
{
   const auto& src_arr = src.GetArray();
   switch (type_name) {
#define TG_META_PRIMITIVE(iden, ty_name)                 \
   case make_name_id(TG_STRING(ty_name)): {              \
      for ([[maybe_unused]] const auto& val : src_arr) { \
         dst.append<ty_name>() = static_cast<ty_name>(TG_JSON_GETTER(iden));   \
      }                                                  \
      break;                                             \
   }
      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      break;
   }
}

void deserialize_primitive_value(const meta::Ref& dst, const std::string_view prop_name, const Name type_name, const rapidjson::Value& src)
{
   const auto prop_name_id = make_name_id(prop_name);
   const auto& val = src[prop_name.data()];

   if (dst.is_array_property(prop_name_id)) {
      auto dst_arr = dst.property_array_ref(prop_name_id);
      deserialize_array(dst_arr, type_name, val);
      return;
   }

   switch (type_name) {
#define TG_META_PRIMITIVE(iden, ty_name)                          \
   case make_name_id(TG_STRING(ty_name)):                         \
      dst.property<ty_name>(prop_name_id) = static_cast<ty_name>(TG_JSON_GETTER(iden)); \
      break;

      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      break;
   }
}

void deserialize_enum_value(const meta::Ref& dst, const Name property_name, const meta::Type& type, const rapidjson::Value& src)
{
   if (src.IsString()) {
      const auto value_name = make_name_id(src.GetString());
      for (const auto& mem : type.members) {
         if (!(mem.role_flags & meta::MemberRole::EnumValue))
            continue;

         if (mem.name == value_name) {
            dst.property<int>(property_name) = mem.enum_value.underlying_value;
            return;
         }
      }
   } else if (src.IsInt()) {
      dst.property<int>(property_name) = src.GetInt();
   }
}

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src)
{
   dst.visit_properties([&](const std::string_view name, const Name type_name) {
      if (!src.HasMember(name.data()))
         return;
      
      const auto& info = meta::TypeRegistry::the().type_info(type_name);
      const auto name_id = make_name_id(name);

      switch (info.variant) {
      case meta::TypeVariant::Primitive:
         deserialize_primitive_value(dst, name, type_name, src);
         break;
      case meta::TypeVariant::Class: {
         if (dst.is_array_property(name_id)) {
            auto dst_arr = dst.property_array_ref(name_id);
            deserialize_array_class(dst_arr, src[name.data()]);
         } else {
            deserialize_value(dst.property_ref(name_id), src[name.data()]);
         }
         break;
      }
      case meta::TypeVariant::Enum:
         deserialize_enum_value(dst, name_id, info, src[name.data()]);
         break;
      }
   });
}

bool deserialize(const meta::Ref& dst, io::IReader& reader)
{
   RapidJsonInputStream stream(reader);

   rapidjson::Document doc;
   doc.ParseStream(stream);
   if (doc.HasParseError()) {
      return false;
   }

   deserialize_value(dst, doc);
   return true;
}

}// namespace triglav::json_util
