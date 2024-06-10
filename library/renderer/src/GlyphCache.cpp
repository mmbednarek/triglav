#include "GlyphCache.h"

namespace triglav::renderer {

namespace {

GlyphCache::Hash hash_glyph_properties(const GlyphProperties& properties)
{
   GlyphCache::Hash result{properties.typeface.name()};
   result += 9251446933U * static_cast<u32>(properties.fontSize);
   return result;
}

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
   runes.emplace_back('[');
   runes.emplace_back(']');
   runes.emplace_back('?');
   runes.emplace_back('_');
   runes.emplace_back('!');
   runes.emplace_back('/');
   runes.emplace_back('\\');
   runes.emplace_back(' ');
   runes.emplace_back(281);
   return runes;
}

auto g_runes{make_runes()};

}// namespace

GlyphCache::GlyphCache(graphics_api::Device& device, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager)
{
}

const render_core::GlyphAtlas& GlyphCache::find_glyph_atlas(const GlyphProperties& properties)
{
   auto hash = hash_glyph_properties(properties);
   auto it = m_atlases.find(hash);
   if (it != m_atlases.end()) {
      return it->second;
   }

   auto& typeface = m_resourceManager.get(properties.typeface);
   auto [atlasIt, ok] = m_atlases.emplace(hash, render_core::GlyphAtlas(m_device, typeface, g_runes, properties.fontSize, 512, 512));
   assert(ok);

   return atlasIt->second;
}

}// namespace triglav::renderer