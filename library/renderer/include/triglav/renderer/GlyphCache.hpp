#pragma once

#include "triglav/Int.hpp"
#include "triglav/Name.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <map>

namespace triglav::renderer {

struct GlyphProperties
{
   TypefaceName typeface;
   int fontSize;

   [[nodiscard]] u64 hash() const;
};

class GlyphCache
{
 public:
   using Hash = u64;

   GlyphCache(graphics_api::Device& device, resource::ResourceManager& resourceManager);

   const render_core::GlyphAtlas& find_glyph_atlas(const GlyphProperties& properties);

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   std::map<Hash, render_core::GlyphAtlas> m_atlases;
};

}// namespace triglav::renderer
