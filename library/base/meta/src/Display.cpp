#include "Display.hpp"

#include "TypeRegistry.hpp"

#include "triglav/io/Iterator.hpp"

#include <format>
#include <vector>

namespace triglav::meta {

using namespace name_literals;

namespace {

class StringWriter
{
 public:
   explicit StringWriter(io::IWriter& writer) :
       m_iterator(writer)
   {
   }

   template<typename... T>
   void print(std::format_string<T...> fmt, T&&... args)
   {
      std::format_to(m_iterator, fmt, std::forward<T>(args)...);
   }

 private:
   io::WriterIterator<char> m_iterator;
};


void display_ref(const Ref& ref, StringWriter& writer, int depth);
void display_property_ref(const PropertyRef& ref, StringWriter& writer, const int depth);

template<typename T>
void display_value(const T& value, StringWriter& writer)
{
   if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
      writer.print("\"{}\"", value);
   } else {
      writer.print("{}", value);
   }
}

void display_optional(const OptionalRef& optional_ref, StringWriter& writer, const int depth)
{
   if (!optional_ref.has_value()) {
      writer.print("none");
      return;
   }
   display_ref(optional_ref.get_ref(), writer, depth);
}

void display_map(const MapRef& map_ref, StringWriter& writer, const int depth)
{
   writer.print("{{");
   bool is_first = true;
   Ref key = map_ref.first_key_ref();
   while (!key.is_nullptr()) {
      if (!is_first) {
         writer.print(", ");
      } else {
         is_first = false;
      }
      display_ref(key, writer, depth);
      writer.print(": ");

      display_ref(map_ref.get_ref(key), writer, depth);
      key = map_ref.next_key_ref(key);
   }
   writer.print("}}");
}

void display_array(const ArrayRef& ref, StringWriter& writer, const int depth)
{
   writer.print("[");
   const auto array_count = ref.size();
   bool is_first = true;
   for (MemorySize i = 0; i < array_count; ++i) {
      if (is_first) {
         is_first = false;
      } else {
         writer.print(", ");
      }
      display_ref(ref.at_ref(i), writer, depth);
   }
   writer.print("]");
}

void display_primitive(const PropertyRef& ref, const Name ty, StringWriter& writer)
{
   switch (ty) {
#define TG_META_PRIMITIVE(iden, name)                            \
   case make_name_id(TG_STRING(name)):                           \
      display_value(static_cast<name>(ref.get<name>()), writer); \
      break;
      TG_META_PRIMITIVE_LIST
#undef TG_META_PRIMITIVE
   default:
      writer.print("unknown");
   }
}

void display_enum(const EnumRef& ref, StringWriter& writer)
{
   writer.print("{}", ref.string());
}

void display_class(const ClassRef& ref, StringWriter& writer, const int depth)
{
   writer.print("{{\n");
   for (const PropertyRef property_ref : ref.properties()) {
      for (int i = 0; i < depth + 1; ++i) {
         writer.print(" ");
      }
      writer.print("{}: ", property_ref.identifier());
      display_property_ref(property_ref, writer, depth);
      writer.print("\n");
   }
   for (int i = 0; i < depth; ++i) {
      writer.print(" ");
   }
   writer.print("}}");
}

void display_property_ref(const PropertyRef& ref, StringWriter& writer, const int depth)
{
   switch (ref.ref_kind()) {
   case RefKind::Primitive:
      display_primitive(ref, ref.type(), writer);
      break;
   case RefKind::Class:
      display_class(ref.to_class_ref(), writer, depth + 1);
      break;
   case RefKind::Enum:
      display_enum(ref.to_enum_ref(), writer);
      break;
   case RefKind::Array:
      display_array(ref.to_array_ref(), writer, depth);
      break;
   case RefKind::Map:
      display_map(ref.to_map_ref(), writer, depth);
      break;
   case RefKind::Optional:
      display_optional(ref.to_optional_ref(), writer, depth);
      break;
   }
}

void display_ref(const Ref& ref, StringWriter& writer, const int depth)
{
   display_property_ref(ref.to_property_ref(), writer, depth);
}

}// namespace

void display(const Ref& ref, io::IWriter& writer)
{
   StringWriter str_writer(writer);
   display_ref(ref, str_writer, -1);
}

}// namespace triglav::meta
