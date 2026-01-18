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

template<typename T>
void display_value(const T& value, StringWriter& writer)
{
   if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
      writer.print("\"{}\"", value);
   } else {
      writer.print("{}", value);
   }
}

template<typename T>
void display_array_val(const ArrayRef& values, StringWriter& writer)
{
   writer.print("[");

   bool is_first = true;
   const auto size = values.size();
   for (size_t i = 0; i < size; ++i) {
      if (is_first) {
         is_first = false;
      } else {
         writer.print(", ");
      }
      display_value(values.at<T>(i), writer);
   }

   writer.print("]");
}

void display_array(const ArrayRef& ref, const Name ty, StringWriter& writer)
{
   switch (ty) {
#define TG_META_PRIMITIVE(iden, name)       \
   case make_name_id(TG_STRING(name)):      \
      display_array_val<name>(ref, writer); \
      break;
      TG_META_PRIMITIVE_LIST
#undef TG_META_PRIMITIVE
   default:
      writer.print("unknown");
   }
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

void display_enum(const PropertyRef& ref, const Type& type_info, StringWriter& writer)
{
   const auto value = ref.get<int>();
   const auto it = std::ranges::find_if(type_info.members, [&](const Member& mem) {
      return mem.role_flags & MemberRole::EnumValue && mem.enum_value.underlying_value == value;
   });
   if (it != type_info.members.end()) {
      writer.print("{}", it->identifier);
   }
}

void display_class(const Ref& ref, StringWriter& writer, int depth);

void display_property_ref(const PropertyRef& ref, StringWriter& writer, const int depth)
{
   const auto& info = TypeRegistry::the().type_info(ref.type());

   for (int i = 0; i < depth + 1; ++i) {
      writer.print(" ");
   }
   writer.print("{}: ", ref.identifier());

   if (ref.is_array()) {
      display_array(ref.to_array_ref(), ref.type(), writer);
   } else if (info.variant == TypeVariant::Class) {
      display_class(ref.to_ref(), writer, depth + 1);
   } else if (info.variant == TypeVariant::Enum) {
      display_enum(ref, info, writer);
   } else {
      display_primitive(ref, ref.type(), writer);
   }

   writer.print("\n");
}

void display_class(const Ref& ref, StringWriter& writer, const int depth)
{
   writer.print("{{\n");
   ref.visit_properties([&](const PropertyRef& property_ref) { display_property_ref(property_ref, writer, depth); });
   for (int i = 0; i < depth; ++i) {
      writer.print(" ");
   }
   writer.print("}}");
}

}// namespace

void display(const Ref& ref, io::IWriter& writer)
{
   StringWriter str_writer(writer);
   display_class(ref, str_writer, 0);
}

}// namespace triglav::meta
