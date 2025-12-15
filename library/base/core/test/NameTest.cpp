#include "triglav/Name.hpp"
#include "triglav/test_util/GTest.hpp"

using triglav::MaterialName;
using triglav::ResourceName;
using triglav::ResourceType;
using triglav::TextureName;
using namespace triglav::name_literals;

TEST(NameTest, BasicBehavior)
{
   static constexpr auto tex_name = "engine/textures/test.tex"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(tex_name)>, TextureName>);
   static_assert(decltype(tex_name)::resource_type == ResourceType::Texture);
   static_assert(tex_name.name() == "engine/textures/test.tex"_name);

   static ResourceName typed_tex_name{tex_name};
   ASSERT_EQ(tex_name, typed_tex_name);

   static constexpr auto tex_name_copy = "engine/textures/test.tex"_rc;
   static_assert(decltype(tex_name_copy)::resource_type == ResourceType::Texture);
   ASSERT_EQ(tex_name, tex_name_copy);
   ASSERT_EQ(tex_name_copy, typed_tex_name);

   static constexpr auto other_name = "engine/textures/other.tex"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(other_name)>, TextureName>);
   ASSERT_NE(tex_name, other_name);
   ASSERT_NE(other_name, typed_tex_name);

   static constexpr auto same_path_different_ext = "engine/textures/test.mat"_rc;
   static_assert(std::is_same_v<std::decay_t<decltype(same_path_different_ext)>, MaterialName>);
   static_assert(decltype(same_path_different_ext)::resource_type == ResourceType::Material);
   ASSERT_NE(same_path_different_ext, typed_tex_name);
}