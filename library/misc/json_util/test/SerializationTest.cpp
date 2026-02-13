#include "triglav/testing_core/GTest.hpp"

#include "triglav/ArrayMap.hpp"
#include "triglav/io/DynamicWriter.hpp"
#include "triglav/io/StringReader.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/json_util/Serialize.hpp"
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
   std::optional<int> optional_int;
   std::optional<float> optional_float;
   triglav::ArrayMap<std::string, float> float_map;
   triglav::ArrayMap<Weekday, int> enum_map;
};

#define TG_TYPE(NS) BasicStruct
TG_META_CLASS_BEGIN
TG_META_PROPERTY(foo, int)
TG_META_PROPERTY(bar, std::string)
TG_META_PROPERTY(contained, Contained)
TG_META_PROPERTY(weekday, Weekday)
TG_META_ARRAY_PROPERTY(colors, std::string)
TG_META_OPTIONAL_PROPERTY(optional_int, int)
TG_META_OPTIONAL_PROPERTY(optional_float, float)
TG_META_MAP_PROPERTY(float_map, std::string, float)
TG_META_MAP_PROPERTY(enum_map, Weekday, int)
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

TEST(JsonTest, Deserilization)
{
   static constexpr auto json_string = R"(
{
   "foo": 25,
   "bar": "lorem ipsum",
   "contained": {
      "value": 12.37
   },
   "weekday": "Friday",
   "colors": ["white", "red", "blue"],
   "optional_int": 30,
   "optional_float": null,
   "float_map": {"pi": 3.1415, "e": 2.7183},
   "enum_map": {"Friday": 5, "Sunday": 7}
}
)";
   triglav::io::StringReader reader(json_string);


   BasicStruct basic_struct{};
   basic_struct.float_map = {{"pi", 3.14f}, {"e", 2.71f}};

   [[maybe_unused]] auto it = basic_struct.float_map.find("e");
   ASSERT_EQ(it->second, 2.71f);

   ASSERT_TRUE(triglav::json_util::deserialize(basic_struct.to_meta_ref(), reader));

   ASSERT_EQ(basic_struct.foo, 25);
   ASSERT_EQ(basic_struct.bar, "lorem ipsum");
   ASSERT_EQ(basic_struct.contained.value, 12.37);
   ASSERT_EQ(basic_struct.weekday, Weekday::Friday);
   const std::vector<std::string> expected_colors{"white", "red", "blue"};
   ASSERT_EQ(basic_struct.colors, expected_colors);
   ASSERT_EQ(basic_struct.optional_int, 30);
   ASSERT_FALSE(basic_struct.optional_float.has_value());
   ASSERT_EQ(basic_struct.float_map["pi"], 3.1415f);
   ASSERT_EQ(basic_struct.float_map["e"], 2.7183f);
   ASSERT_EQ(basic_struct.enum_map[Weekday::Friday], 5);
   ASSERT_EQ(basic_struct.enum_map[Weekday::Sunday], 7);
}

TEST(JsonTest, Serialization)
{
   BasicStruct basic_struct{};
   basic_struct.foo = 25;
   basic_struct.bar = "lorem ipsum";
   basic_struct.contained.value = 12.37;
   basic_struct.weekday = Weekday::Friday;
   basic_struct.colors = {"white", "red", "blue"};
   basic_struct.optional_int = 30;
   basic_struct.optional_float = std::nullopt;
   basic_struct.float_map = {{"e", 2.5f}, {"pi", 3.0f}};
   basic_struct.enum_map = {{Weekday::Monday, 1}, {Weekday::Wednesday, 3}};

   // standard
   {
      triglav::io::DynamicWriter writer(2048);
      ASSERT_TRUE(triglav::json_util::serialize(basic_struct.to_meta_ref(), writer));

      static constexpr auto expected_string =
         R"({"foo":25,"bar":"lorem ipsum","contained":{"value":12.37},"weekday":"Friday","colors":["white","red","blue"],"optional_int":30,"optional_float":null,"float_map":{"e":2.5,"pi":3.0},"enum_map":{"Monday":1,"Wednesday":3}})";
      const std::string result{reinterpret_cast<const char*>(writer.data()), writer.size()};
      ASSERT_EQ(expected_string, result);
   }

   // pretty
   {
      triglav::io::DynamicWriter writer(2048);
      ASSERT_TRUE(triglav::json_util::serialize(basic_struct.to_meta_ref(), writer, true));

      static constexpr auto expected_string = R"({
  "foo": 25,
  "bar": "lorem ipsum",
  "contained": {
    "value": 12.37
  },
  "weekday": "Friday",
  "colors": [
    "white",
    "red",
    "blue"
  ],
  "optional_int": 30,
  "optional_float": null,
  "float_map": {
    "e": 2.5,
    "pi": 3.0
  },
  "enum_map": {
    "Monday": 1,
    "Wednesday": 3
  }
})";
      const std::string result{reinterpret_cast<const char*>(writer.data()), writer.size()};
      ASSERT_EQ(expected_string, result);
   }
}
