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

enum class Weekday
{
   Monday,
   Tuesday,
   Wednesday,
   Thursday,
   Friday,
   Saturday,
   Sunday
};

#define TG_TYPE(NS) Contained
TG_META_CLASS_BEGIN
TG_META_PROPERTY(value, double)
TG_META_CLASS_END
#undef TG_TYPE

struct BasicStruct
{
   TG_META_BODY(BasicStruct)
 public:
   int foo;
   std::string bar;
   Contained contained;
   Weekday weekday;
   std::vector<std::string> colors;
};

#define TG_TYPE(NS) BasicStruct
TG_META_CLASS_BEGIN
TG_META_PROPERTY(foo, int)
TG_META_PROPERTY(bar, std::string)
TG_META_PROPERTY(contained, Contained)
TG_META_PROPERTY(weekday, Weekday)
TG_META_ARRAY_PROPERTY(colors, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) Weekday
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Monday)
TG_META_ENUM_VALUE(Tuesday)
TG_META_ENUM_VALUE(Wednesday)
TG_META_ENUM_VALUE(Thursday)
TG_META_ENUM_VALUE(Friday)
TG_META_ENUM_VALUE(Saturday)
TG_META_ENUM_VALUE(Sunday)
TG_META_ENUM_END
#undef TG_TYPE

TEST(JsonTest, Basic)
{
   static constexpr auto json_string = R"(
{
   "foo": 25,
   "bar": "lorem ipsum",
   "contained": {
      "value": 12.37
   },
   "weekday": "Friday",
   "colors": ["white", "red", "blue"]
}
)";
   triglav::io::StringReader reader(json_string);

   BasicStruct basic_struct{};
   ASSERT_TRUE(triglav::json_util::deserialize(basic_struct.to_meta_ref(), reader));

   ASSERT_EQ(basic_struct.foo, 25);
   ASSERT_EQ(basic_struct.bar, "lorem ipsum");
   ASSERT_EQ(basic_struct.contained.value, 12.37);
   ASSERT_EQ(basic_struct.weekday, Weekday::Friday);
   const std::vector<std::string> expected_colors{"white", "red", "blue"};
   ASSERT_EQ(basic_struct.colors, expected_colors);
}
