#include "triglav/io/BufferReader.hpp"
#include "triglav/io/DynamicWriter.hpp"
#include "triglav/io/StringReader.hpp"
#include "triglav/meta/Binary.hpp"
#include "triglav/meta/Meta.hpp"
#include "triglav/testing_core/GTest.hpp"

struct Foo
{
   TG_META_STRUCT_BODY(Foo)

   int i_val;
   float f_val;
   std::string s_val;
};

#define TG_TYPE(NS) Foo
TG_META_CLASS_BEGIN
TG_META_PROPERTY(i_val, int)
TG_META_PROPERTY(f_val, float)
TG_META_PROPERTY(s_val, std::string)
TG_META_CLASS_END
#undef TG_TYPE


TEST(MetaSerialization, Simple)
{
   Foo foo;
   foo.i_val = 230;
   foo.f_val = 7.56f;
   foo.s_val = "hello";

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, foo.to_meta_ref());

   Foo read;
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(foo.i_val, read.i_val);
   ASSERT_EQ(foo.f_val, read.f_val);
   ASSERT_EQ(foo.s_val, read.s_val);
}

struct TestArray
{
   TG_META_STRUCT_BODY(TestArray)
   std::vector<std::string> values;
};

#define TG_TYPE(NS) TestArray
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(values, std::string)
TG_META_CLASS_END
#undef TG_TYPE

TEST(MetaSerialization, Array)
{
   TestArray target;
   target.values.emplace_back("foo");
   target.values.emplace_back("bar");
   target.values.emplace_back("hello");

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, target.to_meta_ref());

   TestArray read;
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(read.values.size(), 3ull);
   ASSERT_EQ(read.values[0], "foo");
   ASSERT_EQ(read.values[1], "bar");
   ASSERT_EQ(read.values[2], "hello");
}

struct TestMap
{
   TG_META_STRUCT_BODY(TestMap)
   std::map<std::string, triglav::u32> values;
};

#define TG_TYPE(NS) TestMap
TG_META_CLASS_BEGIN
TG_META_MAP_PROPERTY(values, std::string, triglav::u32)
TG_META_CLASS_END
#undef TG_TYPE

TEST(MetaSerialization, Map)
{
   TestMap target;
   target.values["hello"] = 4;
   target.values["world"] = 16;

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, target.to_meta_ref());

   TestMap read;
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(read.values.size(), 2ull);
   ASSERT_EQ(read.values["hello"], 4u);
   ASSERT_EQ(read.values["world"], 16u);
}

struct TestInner
{
   TG_META_STRUCT_BODY(TestInner)
   triglav::u8 small;
   triglav::u64 big;
};

#define TG_TYPE(NS) TestInner
TG_META_CLASS_BEGIN
TG_META_PROPERTY(small, triglav::u8)
TG_META_PROPERTY(big, triglav::u64)
TG_META_CLASS_END
#undef TG_TYPE

struct TestOuter
{
   TG_META_STRUCT_BODY(TestOuter)
   triglav::u32 outer_val;
   TestInner inner_a;
   TestInner inner_b;
};

#define TG_TYPE(NS) TestOuter
TG_META_CLASS_BEGIN
TG_META_PROPERTY(outer_val, triglav::u32)
TG_META_PROPERTY(inner_a, TestInner)
TG_META_PROPERTY(inner_b, TestInner)
TG_META_CLASS_END
#undef TG_TYPE

TEST(MetaSerialization, InnerStruct)
{
   TestOuter target{};
   target.outer_val = 1;
   target.inner_a.small = 2;
   target.inner_a.big = 3;
   target.inner_b.small = 4;
   target.inner_b.big = 5;

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, target.to_meta_ref());

   TestOuter read{};
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(read.outer_val, 1u);
   ASSERT_EQ(read.inner_a.small, 2u);
   ASSERT_EQ(read.inner_a.big, 3u);
   ASSERT_EQ(read.inner_b.small, 4u);
   ASSERT_EQ(read.inner_b.big, 5u);
}

struct TestStructArray
{
   TG_META_STRUCT_BODY(TestStructArray)
   std::vector<TestInner> values;
};

#define TG_TYPE(NS) TestStructArray
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(values, TestInner)
TG_META_CLASS_END
#undef TG_TYPE

TEST(MetaSerialization, StructArray)
{
   TestStructArray target{};
   target.values.emplace_back(1, 2);
   target.values.emplace_back(3, 4);
   target.values.emplace_back(5, 6);

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, target.to_meta_ref());

   TestStructArray read{};
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(read.values.size(), 3u);
   ASSERT_EQ(read.values[0].small, 1u);
   ASSERT_EQ(read.values[0].big, 2u);
   ASSERT_EQ(read.values[1].small, 3u);
   ASSERT_EQ(read.values[1].big, 4u);
   ASSERT_EQ(read.values[2].small, 5u);
   ASSERT_EQ(read.values[2].big, 6u);
}

struct TestStructMap
{
   TG_META_STRUCT_BODY(TestStructMap)
   std::map<triglav::u32, TestInner> values;
};

#define TG_TYPE(NS) TestStructMap
TG_META_CLASS_BEGIN
TG_META_MAP_PROPERTY(values, triglav::u32, TestInner)
TG_META_CLASS_END
#undef TG_TYPE

TEST(MetaSerialization, StructMap)
{
   TestStructMap target{};
   target.values[25] = {1, 2};
   target.values[45] = {3, 4};
   target.values[1002] = {5, 6};

   triglav::io::DynamicWriter writer;
   triglav::meta::serialize_binary(writer, target.to_meta_ref());

   TestStructMap read{};
   triglav::io::BufferReader reader(writer.span());
   triglav::meta::deserialize_binary(reader, read.to_meta_ref());

   ASSERT_EQ(read.values.size(), 3u);
   ASSERT_EQ(read.values[25].small, 1u);
   ASSERT_EQ(read.values[25].big, 2u);
   ASSERT_EQ(read.values[45].small, 3u);
   ASSERT_EQ(read.values[45].big, 4u);
   ASSERT_EQ(read.values[1002].small, 5u);
   ASSERT_EQ(read.values[1002].big, 6u);
}
