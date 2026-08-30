#include "Binary.hpp"

#include "triglav/io/Serializer.hpp"

namespace triglav::meta {

bool serialize_binary_class(io::Serializer& writer, const ClassRef& ref);
bool serialize_binary_property_ref(io::Serializer& writer, const PropertyRef& ref);

#define TG_BINARY_WRITER_char(value) writer.write_i8(value)
#define TG_BINARY_WRITER_int(value) writer.write_i32(value)
#define TG_BINARY_WRITER_triglav__i8(value) writer.write_i8(value)
#define TG_BINARY_WRITER_triglav__u8(value) writer.write_u8(value)
#define TG_BINARY_WRITER_triglav__i16(value) writer.write_i16(value)
#define TG_BINARY_WRITER_triglav__u16(value) writer.write_u16(value)
#define TG_BINARY_WRITER_triglav__i32(value) writer.write_i32(value)
#define TG_BINARY_WRITER_triglav__u32(value) writer.write_u32(value)
#define TG_BINARY_WRITER_triglav__i64(value) writer.write_i64(value)
#define TG_BINARY_WRITER_triglav__u64(value) writer.write_u64(value)
#define TG_BINARY_WRITER_float(value) writer.write_float(value)
#define TG_BINARY_WRITER_double(value) writer.write_double(value)
#define TG_BINARY_WRITER_std__string(value) writer.write_string(value)
#define TG_BINARY_WRITER_std__string_view(value) writer.write_string(value)
#define TG_BINARY_WRITER_triglav__Vector2(value) writer.write_vec2(value)
#define TG_BINARY_WRITER_triglav__Vector3(value) writer.write_vec3(value)
#define TG_BINARY_WRITER_triglav__Vector4(value) writer.write_vec4(value)
#define TG_BINARY_WRITER_triglav__Matrix4x4(value) writer.write_matrix4x4(value)
#define TG_BINARY_WRITER_triglav__Quaternion(value) writer.write_quaternion(value)
#define TG_BINARY_WRITER_triglav__Name(value) writer.write_name(value)
#define TG_BINARY_WRITER(x) TG_CONCAT(TG_BINARY_WRITER_, x)

bool serialize_binary_primitive(io::Serializer& writer, const PropertyRef& ref)
{
   switch (ref.type()) {

#define TG_META_PRIMITIVE(iden, type)                           \
   case make_name_id(TG_STRING(type)):                          \
      if (!TG_BINARY_WRITER(iden)(ref.get<type>()).has_value()) \
         return false;                                          \
      break;

      TG_META_PRIMITIVE_LIST
#undef TG_META_PRIMITIVE

#define TG_RESOURCE_TYPE(resname, ext, cpp_type, loading_stage)                    \
   case make_name_id(TG_STRING(triglav::TG_CONCAT(resname, Name))):                \
      if (!writer.write_name(ref.get<triglav::TG_CONCAT(resname, Name)>().name())) \
         return false;                                                             \
      break;
      TG_RESOURCE_TYPE_LIST
#undef TG_RESOURCE_TYPE

   default:
      return false;
   }

   return true;
}

bool serialize_binary_ref(io::Serializer& writer, const Ref& ref)
{
   return serialize_binary_property_ref(writer, ref.to_property_ref());
}

bool serialize_binary_enum(io::Serializer& writer, const EnumRef& ref)
{
   return writer.write_i32(ref.value()).has_value();
}

bool serialize_binary_array(io::Serializer& writer, const ArrayRef& ref)
{
   if (!writer.write_u32(static_cast<u32>(ref.size())).has_value())
      return false;

   for (size_t i = 0; i < ref.size(); ++i) {
      if (!serialize_binary_ref(writer, ref.at_ref(i)))
         return false;
   }
   return true;
}

bool serialize_binary_map(io::Serializer& writer, const MapRef& ref)
{
   if (!writer.write_u32(static_cast<u32>(ref.count())).has_value())
      return false;

   auto key = ref.first_key_ref();
   while (!key.is_nullptr()) {
      if (!serialize_binary_ref(writer, key))
         return false;

      const auto value = ref.get_ref(key);
      if (!serialize_binary_ref(writer, value))
         return false;

      key = ref.next_key_ref(key);
   }

   return true;
}

bool serialize_binary_optional(io::Serializer& writer, const OptionalRef& ref)
{
   if (!ref.has_value()) {
      if (!writer.write_u8(0).has_value())
         return false;
      return true;
   }

   if (!writer.write_u8(1).has_value())
      return false;

   return serialize_binary_ref(writer, ref.get_ref());
}

bool serialize_binary_property_ref(io::Serializer& writer, const PropertyRef& ref)
{
   switch (ref.ref_kind()) {
   case RefKind::Primitive:
      return serialize_binary_primitive(writer, ref);
   case RefKind::Class:
      return serialize_binary_class(writer, ref.to_class_ref());
   case RefKind::Enum:
      return serialize_binary_enum(writer, ref.to_enum_ref());
   case RefKind::Array:
      return serialize_binary_array(writer, ref.to_array_ref());
   case RefKind::Map:
      return serialize_binary_map(writer, ref.to_map_ref());
   case RefKind::Optional:
      return serialize_binary_optional(writer, ref.to_optional_ref());
   }

   return false;
}

bool serialize_binary_class(io::Serializer& writer, const ClassRef& ref)
{
   for (const PropertyRef property_ref : ref.properties()) {
      if (!serialize_binary_property_ref(writer, property_ref))
         return false;
   }

   return true;
}

bool serialize_binary(io::IWriter& writer, const Ref& ref)
{
   io::Serializer serializer(writer);
   return serialize_binary_ref(serializer, ref);
}

}// namespace triglav::meta
