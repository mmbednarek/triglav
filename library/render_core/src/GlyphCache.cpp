#include "GlyphCache.hpp"

namespace triglav::render_core {

// GlyphProperties

u64 GlyphProperties::hash() const
{
   GlyphCache::Hash result{typeface.name()};
   result += 9251446933U * static_cast<u32>(fontSize);
   return result;
}

// GlyphCache

GlyphCache::GlyphCache(graphics_api::Device& device, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager)
{
}

const GlyphAtlas& GlyphCache::find_glyph_atlas(const GlyphProperties& properties)
{
   auto hash = properties.hash();
   auto it = m_atlases.find(hash);
   if (it != m_atlases.end()) {
      return it->second;
   }

   auto& typeface = m_resourceManager.get(properties.typeface);
   auto [atlasIt, ok] = m_atlases.emplace(hash, GlyphAtlas(m_device, typeface, font::Charset::European, properties.fontSize,
                                                           18 * properties.fontSize, 18 * properties.fontSize));
   assert(ok);

   return atlasIt->second;
}

}// namespace triglav::render_core