#include "triglav/meta/Meta.hpp"
#include "triglav/ArrayMap.hpp"
#include "triglav/io/DynamicWriter.hpp"
#include "triglav/meta/Display.hpp"
#include "triglav/meta/TypeRegistry.hpp"
#include "triglav/testing_core/GTest.hpp"

using namespace triglav::name_literals;

namespace example_namespace {

struct ExampleStruct
{
   TG_META_BODY(ExampleStruct)
 public:
   int foo = 5;
   std::string bar = "hello";
   float goo = 2.4f;
};

class ExampleSubClass
{
   TG_META_BODY(ExampleSubClass)
 public:
   double value = 4.5f;
};

enum class ExampleEnum
{
   Foo,
   Bar,
   Gar,
};

class ExampleClass
{
   TG_META_BODY(ExampleClass)
 public:
   static int instance_count;
   int m_value = 20;
   ExampleStruct m_data{};

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

   [[nodiscard]] float indirect_value() const
   {
      return m_hidden_value + 2.0f;
   }

   void set_indirect_value(const float value)
   {
      m_hidden_value = value;
   }

   [[nodiscard]] const ExampleSubClass& sub_class() const
   {
      return m_sub_class;
   }

   void set_sub_class(const ExampleSubClass& val)
   {
      m_sub_class = val;
   }

   float m_hidden_value = 2.0f;
   ExampleSubClass m_sub_class;
   ExampleEnum m_enum_value = ExampleEnum::Gar;
   std::vector<int> m_int_array = {1, 2, 3};
   triglav::ArrayMap<std::string, int> m_mapped_values = {
      {std::pair<std::string, int>{"blue", 10}, std::pair<std::string, int>{"red", 20}}};
   std::optional<float> m_empty_value;
   std::optional<float> m_optional_value = 12.34f;
};

int ExampleClass::instance_count = 0;

}// namespace example_namespace

#define TG_TYPE(NS) NS(example_namespace, ExampleStruct)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(foo, int)
TG_META_PROPERTY(bar, std::string)
TG_META_PROPERTY(goo, float)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(example_namespace, ExampleSubClass)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(value, double)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(example_namespace, ExampleClass)
TG_META_CLASS_BEGIN
TG_META_METHOD_R(get_value, int)
TG_META_METHOD(set_value, int)
TG_META_PROPERTY(m_value, int)
TG_META_PROPERTY(m_data, example_namespace::ExampleStruct)
TG_META_INDIRECT(indirect_value, float)
TG_META_INDIRECT_REF(sub_class, example_namespace::ExampleSubClass)
TG_META_PROPERTY(m_enum_value, example_namespace::ExampleEnum)
TG_META_ARRAY_PROPERTY(m_int_array, int)
TG_META_MAP_PROPERTY(m_mapped_values, std::string, int)
TG_META_OPTIONAL_PROPERTY(m_empty_value, float)
TG_META_OPTIONAL_PROPERTY(m_optional_value, float)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(example_namespace, ExampleEnum)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Foo)
TG_META_ENUM_VALUE(Bar)
TG_META_ENUM_VALUE_STR(Gar, "G_A_R")
TG_META_ENUM_END
#undef TG_TYPE

TEST(MetaTest, BasicType)
{
   {
      example_namespace::ExampleClass example;
      example.m_value = 100;

      ASSERT_EQ(example_namespace::ExampleClass::instance_count, 1);

      auto obj_ref = example.to_meta_ref();

      triglav::io::DynamicWriter dyn_writer;
      triglav::meta::display(obj_ref, dyn_writer);

      std::string obj_serialized(reinterpret_cast<const char*>(dyn_writer.data()), dyn_writer.size());
      static constexpr auto expected_str = R"({
 m_value: 100
 m_data: {
  foo: 5
  bar: "hello"
  goo: 2.4
 }
 indirect_value: 4
 sub_class: {
  value: 4.5
 }
 m_enum_value: G_A_R
 m_int_array: [1, 2, 3]
 m_mapped_values: {"blue": 10, "red": 20}
 m_empty_value: none
 m_optional_value: 12.34
})";
      ASSERT_EQ(obj_serialized, expected_str);

      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 100);
      obj_ref.call<void>("set_value"_name, 200);
      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 200);
      ASSERT_EQ(obj_ref.property<int>("m_value"_name), 200);
      ASSERT_EQ(obj_ref.property<float>("indirect_value"_name), 4.0f);

      obj_ref.property<float>("indirect_value"_name) = 12.34f;
      obj_ref.property<int>("m_value"_name) = 300;
      ASSERT_EQ(obj_ref.call<int>("get_value"_name), 300);
      ASSERT_EQ(obj_ref.property<float>("indirect_value"_name), 14.34f);
      ASSERT_EQ(obj_ref.property<example_namespace::ExampleSubClass>("sub_class"_name)->value, 4.5f);

      auto map_ref = obj_ref.property_map_ref("m_mapped_values"_name);
      map_ref.set(std::string{"foo"}, 10);
      map_ref.set(std::string{"bar"}, 20);

      const int map_foo = map_ref.at<std::string, int>("bar");
      ASSERT_EQ(map_foo, 20);

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
