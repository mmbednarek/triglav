#include "Deserialize.hpp"

#include "JsonUtil.hpp"

#include "triglav/meta/TypeRegistry.hpp"

namespace triglav::json_util {

using namespace name_literals;

void deserialize_primitive_value(const meta::Ref& dst, const std::string_view name, Name type_name, const rapidjson::Value& src)
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

void deserialize_value(const meta::Ref& dst, const rapidjson::Value& src)
{
   dst.visit_properties([&](const std::string_view name, const Name type_name) {
      const auto& info = meta::TypeRegistry::the().type_info(type_name);

      if (info.variant == meta::TypeVariant::Class) {
         deserialize_value(dst.property_ref(make_name_id(name)), src[name.data()]);
      } else {
         deserialize_primitive_value(dst, name, type_name, src);
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
