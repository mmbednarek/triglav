#include "Deserialize.hpp"

#include "JsonUtil.hpp"

#include "triglav/meta/TypeRegistry.hpp"

namespace triglav::json_util {

using namespace name_literals;

Vector2 read_vector2(const rapidjson::Value& src)
{
   const auto& arr = src.GetArray();
   const auto& x = arr[0];
   const auto& y = arr[1];
   return {x.GetFloat(), y.GetFloat()};
}

Vector3 read_vector3(const rapidjson::Value& src)
{
   const auto& arr = src.GetArray();
   const auto& x = arr[0];
   const auto& y = arr[1];
   const auto& z = arr[2];
   return {x.GetFloat(), y.GetFloat(), z.GetFloat()};
}

Vector4 read_vector4(const rapidjson::Value& src)
{
   const auto& arr = src.GetArray();
   const auto& x = arr[0];
   const auto& y = arr[1];
   const auto& z = arr[2];
   const auto& w = arr[3];
   return {x.GetFloat(), y.GetFloat(), z.GetFloat(), w.GetFloat()};
}

Matrix4x4 read_matrix4x4(const rapidjson::Value& src)
{
   const auto& arr = src.GetArray();
   auto it = arr.begin();

   Matrix4x4 result{};
   for (int row = 0; row < 4; ++row) {
      for (int column = 0; column < 4; ++column) {
         result[row][column] = it->GetFloat();
         ++it;
      }
   }

   return result;
}

#define TG_JSON_GETTER_char val.GetInt()
#define TG_JSON_GETTER_int val.GetInt()
#define TG_JSON_GETTER_triglav__i8 val.GetInt()
#define TG_JSON_GETTER_triglav__u8 val.GetUint()
#define TG_JSON_GETTER_triglav__i16 val.GetInt()
#define TG_JSON_GETTER_triglav__u16 val.GetUint()
#define TG_JSON_GETTER_triglav__i32 val.GetInt()
#define TG_JSON_GETTER_triglav__u32 val.GetUint()
#define TG_JSON_GETTER_triglav__i64 val.GetInt64()
#define TG_JSON_GETTER_triglav__u64 val.GetUint64()
#define TG_JSON_GETTER_float val.GetFloat()
#define TG_JSON_GETTER_double val.GetDouble()
#define TG_JSON_GETTER_std__string val.GetString()
#define TG_JSON_GETTER_std__string_view \
   std::string_view {}// shouldn't use string views for deserialization
#define TG_JSON_GETTER_triglav__Vector2 read_vector2(val)
#define TG_JSON_GETTER_triglav__Vector3 read_vector3(val)
#define TG_JSON_GETTER_triglav__Vector4 read_vector4(val)
#define TG_JSON_GETTER_triglav__Matrix4x4 read_matrix4x4(val)
#define TG_JSON_GETTER(x) TG_CONCAT(TG_JSON_GETTER_, x)

void deserialize_class(const meta::ClassRef& dst, const rapidjson::Value& src);
void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src);
void deserialize_property_ref(const meta::PropertyRef& dst, const rapidjson::Value& src);

void deserialize_primitive_value(const meta::PropertyRef& dst, const rapidjson::Value& val)
{
   switch (dst.type()) {
#define TG_META_PRIMITIVE(iden, ty_name)                            \
   case make_name_id(TG_STRING(ty_name)):                           \
      dst.set<ty_name>(static_cast<ty_name>(TG_JSON_GETTER(iden))); \
      break;

      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      break;
   }
}

void deserialize_class(const meta::ClassRef& dst, const rapidjson::Value& src)
{
   for (const auto property_ref : dst.properties()) {
      const auto prop_name = property_ref.identifier().data();
      if (!src.HasMember(prop_name))
         return;

      deserialize_property_ref(property_ref, src[prop_name]);
   }
}

void deserialize_enum(const meta::EnumRef& dst, const rapidjson::Value& src)
{
   if (src.IsString()) {
      dst.set_string(src.GetString());
   } else if (src.IsInt()) {
      dst.set<int>(src.GetInt());
   }
}

void deserialize_array(const meta::ArrayRef& dst, const rapidjson::Value& src)
{
   const auto& src_arr = src.GetArray();
   for (const auto& val : src_arr) {
      deserialize_value(dst.append_ref(), val);
   }
}

void deserialize_map(const meta::MapRef& dst, const rapidjson::Value& src)
{
   const auto& src_obj = src.GetObj();

   if (dst.key_type() != "std::string"_name) {
      // if key is not string we assume it's an enum

      for (const auto& val : src_obj) {
         std::string key_name = val.name.GetString();
         int enum_val = meta::enum_string_to_value(dst.key_type(), key_name);
         if (enum_val == -1)
            continue;

         meta::Ref key_ref(&enum_val, dst.key_type());
         deserialize_value(dst.get_ref(key_ref), val.value);
      }

      return;
   }

   for (const auto& val : src_obj) {
      std::string key_name = val.name.GetString();
      meta::Ref key_ref(&key_name, "std::string"_name);

      deserialize_value(dst.get_ref(key_ref), val.value);
   }
}

void deserialize_optional(const meta::OptionalRef& dst, const rapidjson::Value& src)
{
   if (src.IsNull()) {
      dst.reset();
      return;
   }
   deserialize_value(dst.get_ref(), src);
}

void deserialize_property_ref(const meta::PropertyRef& dst, const rapidjson::Value& src)
{
   switch (dst.ref_kind()) {
   case meta::RefKind::Primitive:
      deserialize_primitive_value(dst, src);
      break;
   case meta::RefKind::Class:
      deserialize_class(dst.to_class_ref(), src);
      break;
   case meta::RefKind::Enum:
      deserialize_enum(dst.to_enum_ref(), src);
      break;
   case meta::RefKind::Array:
      deserialize_array(dst.to_array_ref(), src);
      break;
   case meta::RefKind::Map:
      deserialize_map(dst.to_map_ref(), src);
      break;
   case meta::RefKind::Optional:
      deserialize_optional(dst.to_optional_ref(), src);
      break;
   }
}

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src)
{
   deserialize_property_ref(dst.to_property_ref(), src);
}

bool deserialize(const meta::ClassRef& dst, io::IReader& reader)
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
