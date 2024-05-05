#include <gtest/gtest.h>

#include "triglav/Name.hpp"

using triglav::ResourceName;
using triglav::TextureName;
using triglav::ResourceType;
using namespace triglav::name_literals;

TEST(NameTest, BasicBehavior)
{
   static constexpr auto texName = "engine/textures/test.tex"_rc;
   ASSERT_EQ(texName.type(), ResourceType::Texture);

   static TextureName typedTexName{texName};
   ASSERT_EQ(texName, typedTexName);

   static constexpr auto texNameCopy = "engine/textures/test.tex"_rc;
   ASSERT_EQ(texNameCopy.type(), ResourceType::Texture);
   ASSERT_EQ(texName, texNameCopy);
   ASSERT_EQ(texNameCopy, typedTexName);

   static constexpr auto otherName = "engine/textures/other.tex"_rc;
   ASSERT_EQ(otherName.type(), ResourceType::Texture);
   ASSERT_NE(texName, otherName);
   ASSERT_NE(otherName, typedTexName);
}