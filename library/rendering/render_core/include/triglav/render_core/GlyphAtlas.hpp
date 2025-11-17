#pragma once

#include "triglav/Math.hpp"
#include "triglav/String.hpp"
#include "triglav/font/Charset.hpp"
#include "triglav/font/Typeface.hpp"
#include "triglav/graphics_api/Texture.hpp"

#include <map>

namespace triglav::render_core {

struct GlyphVertex
{
   Vector2 position;
   Vector2 tex_coord;
};

struct TextMetric
{
   float width;
   float height;
};

struct GlyphInfo
{
   Vector2 tex_coord_top_left;
   Vector2 tex_coord_bottom_right;
   Vector2 size;
   Vector2 advance;
   Vector2 padding;
};

class GlyphAtlas
{
 public:
   GlyphAtlas(graphics_api::Device& device, const font::Typeface& typeface, const font::Charset& atlas_runes, int glyph_size,
              uint32_t width, uint32_t height);

   [[nodiscard]] std::vector<GlyphVertex> create_glyph_vertices(StringView text, TextMetric* out_metric = nullptr) const;
   [[nodiscard]] const graphics_api::Texture& texture() const;
   [[nodiscard]] graphics_api::Texture& texture();
   [[nodiscard]] TextMetric measure_text(StringView text) const;
   [[nodiscard]] u32 find_rune_index(StringView text, float offset) const;
   [[nodiscard]] const graphics_api::Buffer& storage_buffer() const;

 private:
   float m_glyph_size{};
   graphics_api::Texture m_texture;
   graphics_api::Buffer m_glyph_storage_buffer;
   std::map<font::Rune, GlyphInfo> m_glyph_infos;
};

}// namespace triglav::render_core