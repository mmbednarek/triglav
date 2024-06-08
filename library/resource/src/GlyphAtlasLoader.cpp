#include "GlyphAtlasLoader.h"

#include "ResourceManager.h"

#include "triglav/Name.hpp"
#include "triglav/io/File.h"

#include <ryml.hpp>
#include <string_view>

namespace triglav::resource {

namespace {

std::vector<font::Rune> make_runes()
{
   std::vector<font::Rune> runes{};
   for (font::Rune ch = 'A'; ch <= 'Z'; ++ch) {
      runes.emplace_back(ch);
   }
   for (font::Rune ch = 'a'; ch <= 'z'; ++ch) {
      runes.emplace_back(ch);
   }
   for (font::Rune ch = '0'; ch <= '9'; ++ch) {
      runes.emplace_back(ch);
   }
   runes.emplace_back('.');
   runes.emplace_back(':');
   runes.emplace_back('-');
   runes.emplace_back(',');
   runes.emplace_back('(');
   runes.emplace_back(')');
   runes.emplace_back(' ');
   runes.emplace_back(281);
   return runes;
}

auto g_runes{make_runes()};

}// namespace

render_core::GlyphAtlas Loader<ResourceType::GlyphAtlas>::load_gpu(ResourceManager& resourceManager, graphics_api::Device& device,
                                                                   const io::Path& path)
{
   auto file = io::read_whole_file(path);
   assert(not file.empty());

   auto tree =
      ryml::parse_in_place(c4::substr{const_cast<char*>(path.string().data()), path.string().size()}, c4::substr{file.data(), file.size()});

   auto typeface = tree["typeface"].val();
   auto typefaceName = make_rc_name(std::string_view{typeface.str, typeface.len});
   auto sizeStr = tree["size"].val();
   auto size = std::stoi(sizeStr.str);

   return render_core::GlyphAtlas{device, resourceManager.get<ResourceType::Typeface>(typefaceName), g_runes, size, 512, 512};
}

}// namespace triglav::resource
