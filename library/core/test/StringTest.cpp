#include "triglav/String.hpp"
#include "triglav/Format.hpp"

#include <gtest/gtest.h>

using namespace triglav::string_literals;

TEST(StringTest, BasicASCII)
{
   triglav::String example{"Hello World"};
   ASSERT_EQ(example, "Hello World");
   ASSERT_EQ(example.size(), 11ull);
   ASSERT_EQ(example.rune_count(), 11ull);

   auto it = example.begin();
   ASSERT_EQ(*it, "H"_rune);
   ++it;
   ASSERT_EQ(*it, "e"_rune);
   ++it;
   ASSERT_EQ(*it, "l"_rune);
   ++it;
   ASSERT_EQ(*it, "l"_rune);
   ++it;
   ASSERT_EQ(*it, "o"_rune);

   example.append_rune(" "_rune);
   example.append_rune("J"_rune);
   example.append_rune("o"_rune);
   example.append_rune("h"_rune);
   example.append_rune("n"_rune);

   ASSERT_EQ(example, "Hello World John");

   example.append("ny"_strv);

   const auto strView = example.view();
   ASSERT_EQ(strView, "Hello World Johnny");
   ASSERT_EQ(strView.size(), 18ull);
   ASSERT_EQ(strView.rune_count(), 18ull);

   example.shrink_by(3);
   ASSERT_EQ(example, "Hello World Joh");
}

TEST(StringTest, BasicUTF8)
{
   triglav::String example{"Zażółć gęślą jaźń"};
   ASSERT_EQ(example, "Zażółć gęślą jaźń");
   ASSERT_EQ(example.size(), 26ull);
   ASSERT_EQ(example.rune_count(), 17ull);

   auto it = example.begin();
   ASSERT_EQ(*it, "Z"_rune);
   ++it;
   ASSERT_EQ(*it, "a"_rune);
   ++it;
   ASSERT_EQ(*it, "ż"_rune);
   ++it;
   ASSERT_EQ(*it, "ó"_rune);
   ++it;
   ASSERT_EQ(*it, "ł"_rune);
   ++it;
   ASSERT_EQ(*it, "ć"_rune);

   example.append_rune(" "_rune);
   example.append_rune("ź"_rune);
   example.append_rune("d"_rune);
   example.append_rune("ź"_rune);
   example.append_rune("b"_rune);
   example.append_rune("ł"_rune);
   example.append_rune("o"_rune);

   ASSERT_EQ(example, "Zażółć gęślą jaźń źdźbło");
   ASSERT_EQ(example.size(), 36ull);
   ASSERT_EQ(example.rune_count(), 24ull);

   example.append(" żółć"_strv);

   const auto strView = example.view();
   ASSERT_EQ(strView, "Zażółć gęślą jaźń źdźbło żółć");
   ASSERT_EQ(strView.size(), 45ull);
   ASSERT_EQ(strView.rune_count(), 29ull);

   example.shrink_by(2);

   ASSERT_EQ(example, "Zażółć gęślą jaźń źdźbło żó");
}

TEST(StringTest, CharInserterTest)
{
   triglav::String msg{"hello"};
   std::array<char, 3> chars{'A', 'B', 'C'};
   std::copy(chars.begin(), chars.end(), triglav::char_inserter(msg));
   ASSERT_EQ(msg, "helloABC");
}

TEST(StringTest, RuneInserterTest)
{
   triglav::String msg{"hello"};
   std::array<triglav::Rune, 4> runes{80, 81, 321, 82};
   std::copy(runes.begin(), runes.end(), triglav::rune_inserter(msg));
   ASSERT_EQ(msg, "helloPQŁR");
}

TEST(StringTest, FormatTest)
{
   // Plain ASCII
   auto msg = triglav::format("Hello, {} my name is {} and I'm {} years old", "John", "Frank", 25);
   ASSERT_EQ(msg, "Hello, John my name is Frank and I'm 25 years old");

   // UTF-8
   msg = triglav::format("Cześć {}, mam na imię {} i mam {} lat", "Bożena", "Łukasz", 28.5);
   ASSERT_EQ(msg, "Cześć Bożena, mam na imię Łukasz i mam 28.5 lat");
}

TEST(StringTest, CopyTest)
{
   const triglav::String msg{"Zażółć gęślą jaźń źdźbło"};
   const triglav::String copied{msg};

   ASSERT_EQ(msg, "Zażółć gęślą jaźń źdźbło");
   ASSERT_EQ(copied, "Zażółć gęślą jaźń źdźbło");

   triglav::String copied2{};
   copied2 = msg;

   ASSERT_EQ(copied2, "Zażółć gęślą jaźń źdźbło");
}

TEST(StringTest, MoveTest)
{
   triglav::String msg{"Zażółć gęślą jaźń źdźbło"};
   triglav::String moved{std::move(msg)};

   ASSERT_EQ(msg, "");
   ASSERT_TRUE(msg.is_empty());
   ASSERT_EQ(moved, "Zażółć gęślą jaźń źdźbło");

   triglav::String doubleMoved{};
   doubleMoved = std::move(moved);

   ASSERT_EQ(moved, "");
   ASSERT_TRUE(moved.is_empty());
   ASSERT_EQ(doubleMoved, "Zażółć gęślą jaźń źdźbło");
}

TEST(StringTest, Shrink)
{
   /*Small string ASCII*/ {
      triglav::String msg{"aaaabbcc"};
      msg.shrink_by(4);
      ASSERT_EQ(msg, "aaaa");
   }

   /*Small string UTF-8*/ {
      triglav::String msg{"łaźt"};
      msg.shrink_by(2);
      ASSERT_EQ(msg, "ła");
   }

   /*Large string ASCII*/ {
      triglav::String msg{"aaaaaaaaaaaaaaaabbbbccc"};
      msg.shrink_by(3);
      ASSERT_EQ(msg, "aaaaaaaaaaaaaaaabbbb");
   }

   /*Large string UTF-8*/ {
      triglav::String msg{"aaaaaaaaaaaaaaaabbłażcó"};
      msg.shrink_by(3);
      ASSERT_EQ(msg, "aaaaaaaaaaaaaaaabbła");
   }

   /*Shrink large to small ASCII*/ {
      triglav::String msg{"aaaaaaaaccccaaaabbbb"};
      msg.shrink_by(12);
      ASSERT_EQ(msg, "aaaaaaaa");
   }

   /*Shrink large to small UTF-8*/ {
      triglav::String msg{"aćałaacćcąaąaębłbż"};
      msg.shrink_by(12);
      ASSERT_EQ(msg, "aćałaa");
   }
}
