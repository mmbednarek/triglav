#include "GlyphCache.hpp"

namespace triglav::render_core {

// GlyphProperties

u64 GlyphProperties::hash() const
{
   GlyphCache::Hash result{typeface.name()};
   result += 9251446933U * static_cast<u32>(font_size);
   return result;
}

// GlyphCache

GlyphCache::GlyphCache(graphics_api::Device& device, resource::ResourceManager& resource_manager) :
    m_device(device),
    m_resource_manager(resource_manager)
{
}

const GlyphAtlas& GlyphCache::find_glyph_atlas(const GlyphProperties& properties)
{
   auto hash = properties.hash();
   auto it = m_atlases.find(hash);
   if (it != m_atlases.end()) {
      return it->second;
   }

   auto& typeface = m_resource_manager.get(properties.typeface);
   auto [atlas_it, ok] = m_atlases.emplace(hash, GlyphAtlas(m_device, typeface, font::Charset::European, properties.font_size,
                                                            19 * properties.font_size, 19 * properties.font_size));
   assert(ok);

   return atlas_it->second;
}

}// namespace triglav::render_core