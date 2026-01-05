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

void display_primitive(const Ref& ref, const Name ty, StringWriter& writer)
{
   if (ty == "std::string"_name) {
      writer.print("\"{}\"", ref.as<std::string>());
      return;
   }
   if (ty == "std::string_view"_name) {
      writer.print("\"{}\"", ref.as<std::string_view>());
      return;
   }


   switch (ty) {
#define TG_META_PRIMITIVE(iden, name)     \
   case make_name_id(TG_STRING(name)):    \
      writer.print("{}", ref.as<name>()); \
      break;

      TG_META_PRIMITIVE_LIST

#undef TG_META_PRIMITIVE
   default:
      writer.print("unknown");
   }
}

void display_internal(const Ref& ref, StringWriter& writer, int depth = 0);

void display_class(const Ref& ref, StringWriter& writer, const int depth)
{
   writer.print("{{\n");
   ref.visit_properties([&](const std::string_view name, const Name /*type_name*/, const size_t /*offset*/) {
      for (int i = 0; i < depth + 1; ++i) {
         writer.print(" ");
      }
      writer.print("{}: ", name);
      display_internal(ref.property_ref(make_name_id(name)), writer, depth + 1);
      writer.print("\n");
   });
   for (int i = 0; i < depth; ++i) {
      writer.print(" ");
   }
   writer.print("}}");
}

void display_internal(const Ref& ref, StringWriter& writer, const int depth)
{
   const auto ty = ref.type();
   const auto& info = TypeRegistry::the().type_info(ty);
   switch (info.variant) {
   case TypeVariant::Primitive:
      display_primitive(ref, ty, writer);
      break;
   case TypeVariant::Class:
      display_class(ref, writer, depth);
      break;
   }
}

}// namespace

void display(const Ref& ref, io::IWriter& writer)
{
   StringWriter str_writer(writer);
   display_internal(ref, str_writer, 0);
}

}// namespace triglav::meta
