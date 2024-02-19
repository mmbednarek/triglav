#include "GlyphAtlas.h"

#include <cstring>
#include <vector>

#include "triglav/graphics_api/Device.h"
#include "triglav/render_core/RenderCore.hpp"

#include <codecvt>
#include <locale>

using triglav::render_core::checkResult;

namespace triglav::renderer {

GlyphAtlas::GlyphAtlas(graphics_api::Device &device, const font::Typeface &typeface,
                       const std::span<font::Rune> atlasRunes, const int glyphSize, const uint32_t width,
                       const uint32_t height) :
    m_glyphSize(static_cast<float>(glyphSize)),
    m_texture(checkResult(device.create_texture(GAPI_FORMAT(R, sRGB), {width, height})))
{
   std::vector<uint8_t> atlasData{};
   atlasData.resize(width * height);

   const auto widthFP  = static_cast<float>(width);
   const auto heightFP = static_cast<float>(height);

   static constexpr uint32_t separation = 3;

   uint32_t left      = separation;
   uint32_t top       = separation;
   uint32_t maxHeight = 0;

   for (const auto rune : atlasRunes) {
      auto glyph  = typeface.render_glyph(glyphSize, rune);
      auto bottom = top + glyph.height;
      assert((bottom + separation) <= height);

      maxHeight  = std::max(maxHeight, glyph.height + separation);
      auto right = left + glyph.width;
      if (right > width) {
         left  = separation;
         right = left + glyph.width;
         top += maxHeight;
         bottom    = top + glyph.height;
         maxHeight = glyph.height + separation;
      }

      m_glyphInfos.emplace(
              rune, GlyphInfo{
                            { static_cast<float>(left) / widthFP,    static_cast<float>(top) / heightFP},
                            {static_cast<float>(right) / widthFP, static_cast<float>(bottom) / heightFP},
                            {                        glyph.width,                          glyph.height},
                            {                     glyph.advanceX,                        glyph.advanceY},
                            {                   glyph.bitmapLeft,                       glyph.bitmapTop}
      });

      for (int y = 0; y < glyph.height; ++y) {
         std::memcpy(&atlasData[left + (top + y) * width], &glyph.data[y * glyph.width], glyph.width);
      }

      left = right + separation;
   }

   m_texture.write(device, atlasData.data());
}

std::vector<GlyphVertex> GlyphAtlas::create_glyph_vertices(const std::string_view text,
                                                           TextMetric *outMetric) const
{
   std::vector<GlyphVertex> vertices;
   vertices.reserve(text.size() * 6);

   float x         = 0.0f;
   float maxHeight = 0.0f;

   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   const auto textUtf16 = converter.from_bytes(text.data());

   for (const auto ch : textUtf16) {
      if (not m_glyphInfos.contains(ch)) {
         x += m_glyphSize;
         continue;
      }

      const auto &info = m_glyphInfos.at(ch);
      if (maxHeight < info.size.y) {
         maxHeight = info.size.y;
      }

      const auto right = x + info.size.x;

      // top left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y},
                            glm::vec2{info.texCoordTopLeft.x, info.texCoordTopLeft.y});

      // bottom left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.texCoordTopLeft.x, info.texCoordBottomRight.y});

      // top right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y},
                            glm::vec2{info.texCoordBottomRight.x, info.texCoordTopLeft.y});

      // top right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y},
                            glm::vec2{info.texCoordBottomRight.x, info.texCoordTopLeft.y});

      // bottom left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.texCoordTopLeft.x, info.texCoordBottomRight.y});

      // bottom right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.texCoordBottomRight.x, info.texCoordBottomRight.y});

      x += info.advance.x;
   }

   if (outMetric != nullptr) {
      outMetric->width  = x;
      outMetric->height = maxHeight;
   }

   return vertices;
}

const graphics_api::Texture &GlyphAtlas::texture() const
{
   return m_texture;
}

TextMetric GlyphAtlas::measure_text(const std::string_view text) const
{
   float width  = 0.0f;
   float height = 0.0f;

   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   const auto textUtf16 = converter.from_bytes(text.data());

   for (const auto ch : textUtf16) {
      if (not m_glyphInfos.contains(ch)) {
         width += m_glyphSize;
         continue;
      }

      const auto &info = m_glyphInfos.at(ch);
      if (info.size.y > height) {
         height = info.size.y;
      }

      width += info.advance.x;
   }

   return TextMetric{.width = width, .height = height};
}

}// namespace renderer