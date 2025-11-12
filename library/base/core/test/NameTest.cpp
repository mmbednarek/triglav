#include "triglav/Name.hpp"
#include "triglav/test_util/GTest.hpp"

using triglav::MaterialName;
using triglav::ResourceName;
using triglav::ResourceType;
using triglav::TextureName;
using namespace triglav::name_literals;

TEST(NameTest, BasicBehavior)
{
   static constexpr auto texName = "engine/textures/test.tex"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(texName)>, TextureName>);
   static_assert(decltype(texName)::resource_type == ResourceType::Texture);

   static ResourceName typedTexName{texName};
   ASSERT_EQ(texName, typedTexName);

   static constexpr auto texNameCopy = "engine/textures/test.tex"_rc;
   static_assert(decltype(texNameCopy)::resource_type == ResourceType::Texture);
   ASSERT_EQ(texName, texNameCopy);
   ASSERT_EQ(texNameCopy, typedTexName);

   static constexpr auto otherName = "engine/textures/other.tex"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(otherName)>, TextureName>);
   ASSERT_NE(texName, otherName);
   ASSERT_NE(otherName, typedTexName);

   static constexpr auto samePathDifferentExt = "engine/textures/test.mat"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(samePathDifferentExt)>, MaterialName>);
   static_assert(decltype(samePathDifferentExt)::resource_type == ResourceType::Material);
   ASSERT_NE(samePathDifferentExt, typedTexName);
}