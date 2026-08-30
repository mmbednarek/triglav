#include "Binary.hpp"

#include "triglav/io/Deserializer.hpp"

namespace triglav::meta {

#define TG_BINARY_READER_char reader.read_i8()
#define TG_BINARY_READER_int reader.read_i32()
#define TG_BINARY_READER_triglav__i8 reader.read_i8()
#define TG_BINARY_READER_triglav__u8 reader.read_u8()
#define TG_BINARY_READER_triglav__i16 reader.read_i16()
#define TG_BINARY_READER_triglav__u16 reader.read_u16()
#define TG_BINARY_READER_triglav__i32 reader.read_i32()
#define TG_BINARY_READER_triglav__u32 reader.read_u32()
#define TG_BINARY_READER_triglav__i64 reader.read_i64()
#define TG_BINARY_READER_triglav__u64 reader.read_u64()
#define TG_BINARY_READER_float reader.read_float()
#define TG_BINARY_READER_double reader.read_double()
#define TG_BINARY_READER_std__string reader.read_string()
#define TG_BINARY_READER_std__string_view reader.read_string()
#define TG_BINARY_READER_triglav__Vector2 reader.read_vec2()
#define TG_BINARY_READER_triglav__Vector3 reader.read_vec3()
#define TG_BINARY_READER_triglav__Vector4 reader.read_vec4()
#define TG_BINARY_READER_triglav__Matrix4x4 reader.read_matrix4x4()
#define TG_BINARY_READER_triglav__Quaternion reader.read_quaternion()
#define TG_BINARY_READER_triglav__Name reader.read_name()
#define TG_BINARY_READER(x) TG_CONCAT(TG_BINARY_READER_, x)


void deserialize_binary_property_ref(io::Deserializer& reader, const PropertyRef& dst);
void deserialize_binary_value(io::Deserializer& reader, const Ref& dst);

void deserialize_binary_primitive_value(io::Deserializer& reader, const PropertyRef& ref)
{
   switch (ref.type()) {

#define TG_META_PRIMITIVE(iden, type)        \
   case make_name_id(TG_STRING(type)):       \
      ref.set<type>(TG_BINARY_READER(iden)); \
      break;
      TG_META_PRIMITIVE_LIST
#undef TG_META_PRIMITIVE

#define TG_RESOURCE_TYPE(name, ext, cpp_type, loading_stage)                                       \
   case make_name_id(TG_STRING(triglav::TG_CONCAT(name, Name))):                                   \
      ref.set<triglav::TG_CONCAT(name, Name)>(triglav::TG_CONCAT(name, Name){reader.read_name()}); \
      break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE

   default:
      break;
   }
}

void deserialize_binary_class(io::Deserializer& reader, const ClassRef& dst)
{
   for (const auto property_ref : dst.properties()) {
      deserialize_binary_property_ref(reader, property_ref);
   }
}

void deserialize_binary_enum(io::Deserializer& reader, const EnumRef& dst)
{
   dst.set<int>(reader.read_i32());
}

void deserialize_binary_array(io::Deserializer& reader, const ArrayRef& dst)
{
   const u32 count = reader.read_u32();
   for (u32 i = 0; i < count; ++i) {
      deserialize_binary_value(reader, dst.append_ref());
   }
}

void deserialize_binary_map(io::Deserializer& reader, const MapRef& dst)
{
   const u32 count = reader.read_u32();

   for (u32 i = 0; i < count; ++i) {
      auto key = make_object(dst.key_type());
      deserialize_binary_value(reader, key);

      auto value = dst.get_ref(key);
      deserialize_binary_value(reader, value);
   }
}

void deserialize_binary_optional(io::Deserializer& reader, const OptionalRef& dst)
{
   const u8 has_value = reader.read_u8();
   if (has_value == 0) {
      dst.reset();
      return;
   }

   deserialize_binary_value(reader, dst.get_ref());
}

void deserialize_binary_property_ref(io::Deserializer& reader, const PropertyRef& dst)
{
   switch (dst.ref_kind()) {
   case RefKind::Primitive:
      deserialize_binary_primitive_value(reader, dst);
      break;
   case RefKind::Class:
      deserialize_binary_class(reader, dst.to_class_ref());
      break;
   case RefKind::Enum:
      deserialize_binary_enum(reader, dst.to_enum_ref());
      break;
   case RefKind::Array:
      deserialize_binary_array(reader, dst.to_array_ref());
      break;
   case RefKind::Map:
      deserialize_binary_map(reader, dst.to_map_ref());
      break;
   case RefKind::Optional:
      deserialize_binary_optional(reader, dst.to_optional_ref());
      break;
   }
}

void deserialize_binary_value(io::Deserializer& reader, const Ref& dst)
{
   deserialize_binary_property_ref(reader, dst.to_property_ref());
}

void deserialize_binary(io::IReader& reader, const Ref& ref)
{
   io::Deserializer deserializer(reader);
   deserialize_binary_value(deserializer, ref);
}

}// namespace triglav::meta
