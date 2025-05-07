#pragma once

#include "triglav/Math.hpp"
#include "triglav/font/Charset.hpp"
#include "triglav/font/Typeface.hpp"
#include "triglav/graphics_api/Texture.hpp"

#include <map>

namespace triglav::render_core {

struct GlyphVertex
{
   Vector2 position;
   Vector2 texCoord;
};

struct TextMetric
{
   float width;
   float height;
};

struct GlyphInfo
{
   Vector2 texCoordTopLeft;
   Vector2 texCoordBottomRight;
   Vector2 size;
   Vector2 advance;
   Vector2 padding;
};

class GlyphAtlas
{
 public:
   GlyphAtlas(graphics_api::Device& device, const font::Typeface& typeface, const font::Charset& atlasRunes, int glyphSize, uint32_t width,
              uint32_t height);

   [[nodiscard]] std::vector<GlyphVertex> create_glyph_vertices(std::string_view text, TextMetric* outMetric = nullptr) const;
   [[nodiscard]] const graphics_api::Texture& texture() const;
   [[nodiscard]] graphics_api::Texture& texture();
   [[nodiscard]] TextMetric measure_text(std::string_view text) const;
   [[nodiscard]] const graphics_api::Buffer& storage_buffer() const;

 private:
   float m_glyphSize{};
   graphics_api::Texture m_texture;
   graphics_api::Buffer m_glyphStorageBuffer;
   std::map<font::Rune, GlyphInfo> m_glyphInfos;
};

}// namespace triglav::render_core