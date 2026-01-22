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

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src);

void deserialize_array_class(const meta::ArrayRef& dst, const rapidjson::Value& src)
{
   const auto& src_arr = src.GetArray();
   for ([[maybe_unused]] const auto& val : src_arr) {
      auto ref = dst.append_ref();
      deserialize_value(ref, val);
   }
}

void deserialize_array(const meta::ArrayRef& dst, const Name type_name, const rapidjson::Value& src)
{
   const auto& src_arr = src.GetArray();
   switch (type_name) {
#define TG_META_PRIMITIVE(iden, ty_name)                                     \
   case make_name_id(TG_STRING(ty_name)): {                                  \
      for ([[maybe_unused]] const auto& val : src_arr) {                     \
         dst.append<ty_name>() = static_cast<ty_name>(TG_JSON_GETTER(iden)); \
      }                                                                      \
      break;                                                                 \
   }
      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      break;
   }
}

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

void deserialize_enum_value(const meta::PropertyRef& dst, const meta::Type& type, const rapidjson::Value& src)
{
   if (src.IsString()) {
      const auto value_name = make_name_id(src.GetString());
      for (const auto& mem : type.members) {
         if (!(mem.role_flags & meta::MemberRole::EnumValue))
            continue;

         if (mem.name == value_name) {
            dst.set<int>(mem.enum_value.underlying_value);
            return;
         }
      }
   } else if (src.IsInt()) {
      dst.set<int>(src.GetInt());
   }
}

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src)
{
   dst.visit_properties([&](const meta::PropertyRef& ref) {
      if (!src.HasMember(ref.identifier().data()))
         return;

      const auto& info = meta::TypeRegistry::the().type_info(ref.type());

      if (ref.is_array()) {
         if (info.variant == meta::TypeVariant::Class) {
            deserialize_array_class(ref.to_array_ref(), src[ref.identifier().data()]);
         } else {
            deserialize_array(ref.to_array_ref(), ref.type(), src[ref.identifier().data()]);
         }

      } else {
         switch (info.variant) {
         case meta::TypeVariant::Primitive:
            deserialize_primitive_value(ref, src[ref.identifier().data()]);
            break;
         case meta::TypeVariant::Class: {
            deserialize_value(ref.to_ref(), src[ref.identifier().data()]);
            break;
         }
         case meta::TypeVariant::Enum:
            deserialize_enum_value(ref, info, src[ref.identifier().data()]);
            break;
         default:
            break;
         }
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
