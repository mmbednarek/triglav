#include "triglav/test_util/GTest.hpp"

#include "triglav/io/StringReader.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/meta/Meta.hpp"

struct Contained
{
   TG_META_BODY(Contained)
 public:
   double value;
};

#define TG_CLASS_NS

#define TG_CLASS_NAME Contained
#define TG_CLASS_IDENTIFIER Contained

TG_META_CLASS_BEGIN
TG_META_PROPERTY(value, double)
TG_META_CLASS_END

#undef TG_CLASS_NAME
#undef TG_CLASS_IDENTIFIER

struct BasicStruct
{
   TG_META_BODY(BasicStruct)
 public:
   int foo;
   std::string bar;
   Contained contained;
};

#define TG_CLASS_NAME BasicStruct
#define TG_CLASS_IDENTIFIER BasicStruct

TG_META_CLASS_BEGIN
TG_META_PROPERTY(foo, int)
TG_META_PROPERTY(bar, std::string)
TG_META_PROPERTY(contained, ::Contained)
TG_META_CLASS_END

#undef TG_CLASS_NAME
#undef TG_CLASS_IDENTIFIER

#undef TG_CLASS_NS

TEST(JsonTest, Basic)
{
   static constexpr auto json_string = R"(
{
   "foo": 25,
   "bar": "lorem ipsum",
   "contained": {
      "value": 12.37
   }
}
)";
   triglav::io::StringReader reader(json_string);

   BasicStruct basic_struct{};
   ASSERT_TRUE(triglav::json_util::deserialize(basic_struct.to_meta_ref(), reader));

   ASSERT_EQ(basic_struct.foo, 25);
   ASSERT_EQ(basic_struct.bar, "lorem ipsum");
   ASSERT_EQ(basic_struct.contained.value, 12.37);
}
