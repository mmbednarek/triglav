#include "Serialize.hpp"

#include "triglav/meta/TypeRegistry.hpp"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace triglav::json_util {

class OutputBuffer
{
 public:
   using Ch = char;

   OutputBuffer(io::IWriter& writer) :
       m_writer(writer)
   {
   }

   void Put(const char c) const
   {
      m_writer.write({reinterpret_cast<const u8*>(&c), 1});
   }

   void PutUnsafe(const char c) const
   {
      this->Put(c);
   }

   void Flush()
   {
      // nothing to do
   }

   void Clear()
   {
      // ignore
   }

 private:
   io::IWriter& m_writer;
};

#define TG_JSON_SETTER_char(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_int(value) writer.Int(value)
#define TG_JSON_SETTER_i8(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_u8(value) writer.Uint(static_cast<u32>(value))
#define TG_JSON_SETTER_i16(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_u16(value) writer.Uint(static_cast<u32>(value))
#define TG_JSON_SETTER_i32(value) writer.Int(value)
#define TG_JSON_SETTER_u32(value) writer.Uint(value)
#define TG_JSON_SETTER_i64(value) writer.Int64(value)
#define TG_JSON_SETTER_u64(value) writer.Uint64(value)
#define TG_JSON_SETTER_float(value) writer.Double(static_cast<double>(value))
#define TG_JSON_SETTER_double(value) writer.Double(value)
#define TG_JSON_SETTER_std__string(value) writer.String(value.data(), value.size(), true)
#define TG_JSON_SETTER_std__string_view(value) \
   writer.String(value.data(), value.size(), true)// shouldn't use string views for deserialization
#define TG_JSON_SETTER(x) TG_CONCAT(TG_JSON_SETTER_, x)

using namespace name_literals;

template<typename TWriter>
bool serialize_property_ref(TWriter& writer, const meta::PropertyRef& ref);

template<typename TWriter>
bool serialize_primitive(TWriter& writer, const meta::PropertyRef& ref)
{
   switch (ref.type()) {
#define TG_META_PRIMITIVE(iden, type)        \
   case make_name_id(TG_STRING(type)):       \
      TG_JSON_SETTER(iden)(ref.get<type>()); \
      break;

      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      return false;
   }
   return true;
}

template<typename TWriter>
bool serialize_class(TWriter& writer, const meta::Ref& ref)
{
   bool result = true;

   writer.StartObject();
   ref.visit_properties([&](const meta::PropertyRef& prop_ref) { result = result && serialize_property_ref(writer, prop_ref); });
   writer.EndObject();

   return result;
}

template<typename TWriter>
bool serialize_enum(TWriter& writer, const meta::PropertyRef& ref)
{
   const auto& info = meta::TypeRegistry::the().type_info(ref.type());
   for (const auto& mem : info.members) {
      if (!(mem.role_flags & meta::MemberRole::EnumValue))
         continue;

      if (mem.enum_value.underlying_value == ref.get<int>()) {
         writer.String(mem.identifier.data(), mem.identifier.size(), true);
         return true;
      }
   }

   return false;
}

template<typename TWriter>
bool serialize_array_primitive(TWriter& writer, const meta::ArrayRef& ref)
{
   switch (ref.type()) {
#define TG_META_PRIMITIVE(iden, type)           \
   case make_name_id(TG_STRING(type)): {        \
      for (size_t i = 0; i < ref.size(); ++i) { \
         TG_JSON_SETTER(iden)(ref.at<type>(i)); \
      }                                         \
      break;                                    \
   }
      TG_META_PRIMITIVE_LIST
#undef TG_META_PRIMITIVE
   default:
      return false;
   }
   return true;
}

template<typename TWriter>
bool serialize_array_class(TWriter& writer, const meta::ArrayRef& ref)
{
   for (size_t i = 0; i < ref.size(); ++i) {
      serialize_class(writer, ref.at_ref(i));
   }
   return true;
}

template<typename TWriter>
bool serialize_array(TWriter& writer, const meta::ArrayRef& ref)
{
   bool is_ok = false;
   writer.StartArray();
   switch (ref.type_variant()) {
   case meta::TypeVariant::Primitive:
      is_ok = serialize_array_primitive(writer, ref);
      break;
   case meta::TypeVariant::Class:
      is_ok = serialize_array_class(writer, ref);
      break;
   default:
      break;
   }
   writer.EndArray();
   return is_ok;
}

template<typename TWriter>
bool serialize_property_ref(TWriter& writer, const meta::PropertyRef& ref)
{
   writer.Key(ref.identifier().data());

   switch (ref.type_variant()) {
   case meta::TypeVariant::Primitive:
      return serialize_primitive(writer, ref);
   case meta::TypeVariant::Class:
      return serialize_class(writer, ref.to_ref());
   case meta::TypeVariant::Enum:
      return serialize_enum(writer, ref);
   case meta::TypeVariant::Array:
      return serialize_array(writer, ref.to_array_ref());
   }

   return false;
}

using JsonWriter = rapidjson::Writer<OutputBuffer>;
using JsonPrettyWriter = rapidjson::PrettyWriter<OutputBuffer>;

bool serialize(const meta::Ref& dst, io::IWriter& writer, const bool pretty_print)
{
   OutputBuffer output_buffer(writer);

   if (pretty_print) {
      JsonPrettyWriter json_writer(output_buffer);
      json_writer.SetIndent(' ', 2);
      return serialize_class(json_writer, dst);
   } else {
      JsonWriter json_writer(output_buffer);
      return serialize_class(json_writer, dst);
   }
}

}// namespace triglav::json_util
