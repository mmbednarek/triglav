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

template<typename TWriter>
void write_vector2(TWriter& writer, Vector2 vec)
{
   writer.StartArray();
   writer.Double(vec.x);
   writer.Double(vec.y);
   writer.EndArray();
}

template<typename TWriter>
void write_vector3(TWriter& writer, Vector3 vec)
{
   writer.StartArray();
   writer.Double(vec.x);
   writer.Double(vec.y);
   writer.Double(vec.z);
   writer.EndArray();
}

template<typename TWriter>
void write_vector4(TWriter& writer, Vector4 vec)
{
   writer.StartArray();
   writer.Double(vec.x);
   writer.Double(vec.y);
   writer.Double(vec.z);
   writer.Double(vec.w);
   writer.EndArray();
}

template<typename TWriter>
void write_matrix4x4(TWriter& writer, Matrix4x4 mat)
{
   writer.StartArray();
   for (int i = 0; i < 4; ++i) {
      writer.Double(mat[i].x);
      writer.Double(mat[i].y);
      writer.Double(mat[i].z);
      writer.Double(mat[i].w);
   }
   writer.EndArray();
}

#define TG_JSON_SETTER_char(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_int(value) writer.Int(value)
#define TG_JSON_SETTER_triglav__i8(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_triglav__u8(value) writer.Uint(static_cast<u32>(value))
#define TG_JSON_SETTER_triglav__i16(value) writer.Int(static_cast<int>(value))
#define TG_JSON_SETTER_triglav__u16(value) writer.Uint(static_cast<u32>(value))
#define TG_JSON_SETTER_triglav__i32(value) writer.Int(value)
#define TG_JSON_SETTER_triglav__u32(value) writer.Uint(value)
#define TG_JSON_SETTER_triglav__i64(value) writer.Int64(value)
#define TG_JSON_SETTER_triglav__u64(value) writer.Uint64(value)
#define TG_JSON_SETTER_float(value) writer.Double(static_cast<double>(value))
#define TG_JSON_SETTER_double(value) writer.Double(value)
#define TG_JSON_SETTER_std__string(value) writer.String(value.data(), value.size(), true)
#define TG_JSON_SETTER_std__string_view(value) \
   writer.String(value.data(), value.size(), true)// shouldn't use string views for deserialization
#define TG_JSON_SETTER_triglav__Vector2(value) write_vector2(writer, value)
#define TG_JSON_SETTER_triglav__Vector3(value) write_vector3(writer, value)
#define TG_JSON_SETTER_triglav__Vector4(value) write_vector4(writer, value)
#define TG_JSON_SETTER_triglav__Matrix4x4(value) write_matrix4x4(writer, value)
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
bool serialize_class(TWriter& writer, const meta::ClassRef& ref)
{
   writer.StartObject();
   for (const meta::PropertyRef property_ref : ref.properties()) {
      writer.Key(property_ref.identifier().data());

      if (!serialize_property_ref(writer, property_ref))
         return false;
   }
   writer.EndObject();

   return true;
}

template<typename TWriter>
bool serialize_ref(TWriter& writer, const meta::Ref& ref)
{
   return serialize_property_ref(writer, ref.to_property_ref());
}

template<typename TWriter>
bool serialize_enum(TWriter& writer, const meta::EnumRef& ref)
{
   writer.String(ref.string().data());
   return true;
}

template<typename TWriter>
bool serialize_array(TWriter& writer, const meta::ArrayRef& ref)
{
   writer.StartArray();
   for (size_t i = 0; i < ref.size(); ++i) {
      if (!serialize_ref(writer, ref.at_ref(i)))
         return false;
   }
   writer.EndArray();
   return true;
}

template<typename TWriter>
bool serialize_map_enum_key(TWriter& writer, const meta::MapRef& ref)
{
   writer.StartObject();

   auto key_ref = ref.first_key_ref();
   while (!key_ref.is_nullptr()) {
      writer.Key(key_ref.to_enum_ref().string().data());

      auto value_ref = ref.get_ref(key_ref);
      if (!serialize_ref(writer, value_ref))
         return false;

      key_ref = ref.next_key_ref(key_ref);
   }
   writer.EndObject();
   return true;
}

template<typename TWriter>
bool serialize_map(TWriter& writer, const meta::MapRef& ref)
{
   if (ref.key_type() != "std::string"_name)
      return serialize_map_enum_key(writer, ref);

   writer.StartObject();

   auto key_ref = ref.first_key_ref();
   while (!key_ref.is_nullptr()) {
      writer.Key(key_ref.as<std::string>().data());

      auto value_ref = ref.get_ref(key_ref);
      if (!serialize_ref(writer, value_ref))
         return false;

      key_ref = ref.next_key_ref(key_ref);
   }
   writer.EndObject();
   return true;
}

template<typename TWriter>
bool serialize_optional(TWriter& writer, const meta::OptionalRef& ref)
{
   if (!ref.has_value()) {
      writer.Null();
      return true;
   }

   return serialize_ref(writer, ref.get_ref());
}

template<typename TWriter>
bool serialize_property_ref(TWriter& writer, const meta::PropertyRef& ref)
{
   switch (ref.ref_kind()) {
   case meta::RefKind::Primitive:
      return serialize_primitive(writer, ref);
   case meta::RefKind::Class:
      return serialize_class(writer, ref.to_class_ref());
   case meta::RefKind::Enum:
      return serialize_enum(writer, ref.to_enum_ref());
   case meta::RefKind::Array:
      return serialize_array(writer, ref.to_array_ref());
   case meta::RefKind::Map:
      return serialize_map(writer, ref.to_map_ref());
   case meta::RefKind::Optional:
      return serialize_optional(writer, ref.to_optional_ref());
   }

   return false;
}

using JsonWriter = rapidjson::Writer<OutputBuffer>;
using JsonPrettyWriter = rapidjson::PrettyWriter<OutputBuffer>;

bool serialize(const meta::ClassRef& dst, io::IWriter& writer, const bool pretty_print)
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
