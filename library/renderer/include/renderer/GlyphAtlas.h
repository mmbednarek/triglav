#pragma once

#include "font/Typeface.h"
#include "graphics_api/Texture.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <map>
#include <span>

namespace renderer {

struct GlyphVertex
{
   glm::vec2 position;
   glm::vec2 texCoord;
};

struct TextMetric
{
   float width;
   float height;
};

struct GlyphInfo
{
   glm::vec2 texCoordTopLeft;
   glm::vec2 texCoordBottomRight;
   glm::vec2 size;
   glm::vec2 advance;
   glm::vec2 padding;
};

class GlyphAtlas
{
 public:
   GlyphAtlas(graphics_api::Device &device, const font::Typeface &typeface, std::span<font::Rune> atlasRunes,
              int glyphSize, uint32_t width, uint32_t height);

   [[nodiscard]] std::vector<GlyphVertex> create_glyph_vertices(std::string_view text, TextMetric* outMetric = nullptr) const;
   [[nodiscard]] const graphics_api::Texture &texture() const;
   [[nodiscard]] TextMetric measure_text(std::string_view text) const;

 private:
   float m_glyphSize{};
   graphics_api::Texture m_texture;
   std::map<font::Rune, GlyphInfo> m_glyphInfos;
};

}// namespace renderer