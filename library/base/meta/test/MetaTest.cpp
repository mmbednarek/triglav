#include "triglav/meta/Meta.hpp"
#include "triglav/meta/TypeRegistry.hpp"
#include "triglav/test_util/GTest.hpp"

using namespace triglav::name_literals;

namespace example_namespace {

class ExampleClass
{
   TG_META_BODY(ExampleClass)
 public:
   static int instance_count;
   int m_value = 20;

   ExampleClass()
   {
      ++instance_count;
   }

   ~ExampleClass()
   {
      --instance_count;
   }

   ExampleClass(const ExampleClass& other) :
       m_value(other.m_value)
   {
      ++instance_count;
   }

   ExampleClass& operator=(const ExampleClass& other)
   {
      m_value = other.m_value;
      return *this;
   }


   [[nodiscard]] int get_value() const
   {
      return m_value;
   }

   void set_value(const int value)
   {
      m_value = value;
   }
};

int ExampleClass::instance_count = 0;

}// namespace example_namespace

#define TG_CLASS_NAME ExampleClass
#define TG_CLASS_NS example_namespace
#define TG_CLASS_IDENTIFIER example_namespace__ExampleClass

TG_META_CLASS_BEGIN
TG_META_METHOD0_R(get_value, int)
TG_META_METHOD1(set_value, int)
TG_META_PROPERTY(m_value, int)
TG_META_CLASS_END

#undef TG_CLASS_NAME
#undef TG_CLASS_NS
#undef TG_CLASS_IDENTIFIER

TEST(MetaTest, BasicType)
{
   {
      example_namespace::ExampleClass example;
      example.m_value = 100;

      ASSERT_EQ(example_namespace::ExampleClass::instance_count, 1);

      auto obj_ref = example.to_meta_ref();

      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 100);
      obj_ref.call<void>("set_value"_name, 200);
      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 200);
      ASSERT_EQ(obj_ref.property<int>("m_value"_name), 200);

      obj_ref.property<int>("m_value"_name) = 300;
      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 300);

      obj_ref.visit_properties([](const std::string_view name, const triglav::Name type_name, const size_t offset) {
         ASSERT_EQ(name, "m_value");
         ASSERT_EQ(type_name, "int"_name);
         ASSERT_EQ(offset, 0ull);
      });

      auto box = triglav::meta::TypeRegistry::the().create_box("example_namespace::ExampleClass"_name);

      ASSERT_EQ(example_namespace::ExampleClass::instance_count, 2);

      ASSERT_EQ(box.call<int>("get_value"_name), 20);
      box.call<void>("set_value"_name, 40);
      ASSERT_EQ(box.call<int>("get_value"_name), 40);

      auto box2 = box;

      ASSERT_EQ(example_namespace::ExampleClass::instance_count, 3);
      ASSERT_EQ(box2.call<int>("get_value"_name), 40);
      box2.call<void>("set_value"_name, 80);
      ASSERT_EQ(box.call<int>("get_value"_name), 40);
      ASSERT_EQ(box2.call<int>("get_value"_name), 80);
   }
   ASSERT_EQ(example_namespace::ExampleClass::instance_count, 0);
}
