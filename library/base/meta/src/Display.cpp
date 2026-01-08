#include "Display.hpp"

#include "TypeRegistry.hpp"

#include "triglav/io/Iterator.hpp"

#include <format>

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

void display_primitive(const Ref& ref, const Name prop_name, const Name ty, StringWriter& writer)
{
   if (ty == "std::string"_name) {
      writer.print("\"{}\"", static_cast<std::string>(ref.property<std::string>(prop_name)));
      return;
   }
   if (ty == "std::string_view"_name) {
      writer.print("\"{}\"", static_cast<std::string_view>(ref.property<std::string_view>(prop_name)));
      return;
   }

   switch (ty) {
#define TG_META_PRIMITIVE(iden, name)                                       \
   case make_name_id(TG_STRING(name)):                                      \
      writer.print("{}", static_cast<name>(ref.property<name>(prop_name))); \
      break;

      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      writer.print("unknown");
   }
}

void display_enum(const Ref& ref, const Name prop_name, const Type& type_info, StringWriter& writer)
{
   const auto value = ref.property<int>(prop_name);
   const auto it = std::ranges::find_if(type_info.members, [&](const ClassMember& mem) {
      return mem.type == ClassMemberType::EnumValue && mem.enum_value.underlying_value == value;
   });
   if (it != type_info.members.end()) {
      writer.print("{}", it->identifier);
   }
}

void display_class(const Ref& ref, StringWriter& writer, const int depth)
{
   writer.print("{{\n");
   ref.visit_properties([&](const std::string_view name, const Name type_name) {
      const auto& info = TypeRegistry::the().type_info(type_name);

      for (int i = 0; i < depth + 1; ++i) {
         writer.print(" ");
      }
      writer.print("{}: ", name);
      if (info.variant == TypeVariant::Class) {
         display_class(ref.property_ref(make_name_id(name)), writer, depth + 1);
      } else if (info.variant == TypeVariant::Enum) {
         display_enum(ref, make_name_id(name), info, writer);
      } else {
         display_primitive(ref, make_name_id(name), type_name, writer);
      }
      writer.print("\n");
   });
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
