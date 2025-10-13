#include "triglav/io/DynamicWriter.hpp"

#include <cstring>
#include <gtest/gtest.h>

using triglav::u8;
using triglav::io::DynamicWriter;

const auto TEST_STRING =
   "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad "
   "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";

TEST(IO_DynamicWriterTests, BasicTest)
{
   DynamicWriter writer;
   ASSERT_EQ(writer.size(), 0u);
   ASSERT_EQ(writer.capacity(), 128u);
   ASSERT_EQ(writer.span().size(), 0u);

   auto write_res = writer.write({reinterpret_cast<const u8*>(TEST_STRING), 16});
   ASSERT_TRUE(write_res.has_value());
   ASSERT_EQ(write_res, 16u);

   ASSERT_EQ(writer.size(), 16u);
   ASSERT_EQ(writer.capacity(), 128u);
   ASSERT_EQ(writer.span().size(), 16u);

   ASSERT_EQ(std::strncmp(reinterpret_cast<const char*>(writer.data()), TEST_STRING, 16), 0);
   ASSERT_EQ(std::strncmp(reinterpret_cast<const char*>(writer.span().data()), TEST_STRING, 16), 0);

   write_res = writer.write({reinterpret_cast<const u8*>(TEST_STRING), 231});
   ASSERT_TRUE(write_res.has_value());
   ASSERT_EQ(write_res, 231u);

   ASSERT_EQ(writer.size(), 247u);
   ASSERT_EQ(writer.capacity(), 256u);
   ASSERT_EQ(writer.span().size(), 247u);

   ASSERT_EQ(std::strncmp(reinterpret_cast<const char*>(writer.data()), TEST_STRING, 16), 0);
   ASSERT_EQ(std::strncmp(reinterpret_cast<const char*>(writer.data()) + 16, TEST_STRING, 231), 0);
}