#include "Deserialize.hpp"

#include "JsonUtil.hpp"

#include "triglav/meta/TypeRegistry.hpp"

namespace triglav::json_util {

using namespace name_literals;

void deserialize_primitive_value(const meta::Ref& dst, const std::string_view name, const Name type_name, const rapidjson::Value& src)
{
   if (!src.HasMember(name.data()))
      return;

   switch (type_name) {
   case "int"_name:
      [[fallthrough]];
   case "i32"_name:
      dst.property<int>(make_name_id(name)) = src[name.data()].GetInt();
      break;
   case "unsigned"_name:
      [[fallthrough]];
   case "u32"_name:
      dst.property<u32>(make_name_id(name)) = src[name.data()].GetUint();
      break;
   case "i64"_name:
      dst.property<i64>(make_name_id(name)) = src[name.data()].GetInt64();
      break;
   case "u64"_name:
      dst.property<u64>(make_name_id(name)) = src[name.data()].GetUint64();
      break;
   case "float"_name:
      dst.property<float>(make_name_id(name)) = src[name.data()].GetFloat();
      break;
   case "double"_name:
      dst.property<double>(make_name_id(name)) = src[name.data()].GetDouble();
      break;
   case "std::string"_name:
      dst.property<std::string>(make_name_id(name)) = src[name.data()].GetString();
      break;
   }
}

void deserialize_enum_value(const meta::Ref& dst, const Name property_name, const meta::Type& type, const rapidjson::Value& src)
{
   if (src.IsString()) {
      const auto value_name = make_name_id(src.GetString());
      for (const auto& mem : type.members) {
         if (mem.type != meta::ClassMemberType::EnumValue)
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
      const auto& info = meta::TypeRegistry::the().type_info(type_name);
      const auto name_id = make_name_id(name);

      switch (info.variant) {
      case meta::TypeVariant::Primitive:
         deserialize_primitive_value(dst, name, type_name, src);
         break;
      case meta::TypeVariant::Class:
         deserialize_value(dst.property_ref(name_id), src[name.data()]);
         break;
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
