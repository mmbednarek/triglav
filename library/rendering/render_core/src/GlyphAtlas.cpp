#include "GlyphAtlas.hpp"

#include "RenderCore.hpp"

#include "triglav/graphics_api/Device.hpp"

#include <cstring>
#include <locale>
#include <vector>

namespace triglav::render_core {

namespace gapi = graphics_api;

GlyphAtlas::GlyphAtlas(gapi::Device& device, const font::Typeface& typeface, const font::Charset& atlasRunes, const int glyphSize,
                       const uint32_t width, const uint32_t height) :
    m_glyphSize(static_cast<float>(glyphSize)),
    m_texture(checkResult(device.create_texture(GAPI_FORMAT(R, UNorm8), {width, height}))),
    m_glyphStorageBuffer(
       GAPI_CHECK(device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst | gapi::BufferUsage::TransferSrc,
                                       sizeof(GlyphInfo) * atlasRunes.count())))
{
   std::vector<uint8_t> atlasData{};
   atlasData.resize(width * height);

   const auto widthFP = static_cast<float>(width);
   const auto heightFP = static_cast<float>(height);

   static constexpr uint32_t separation = 4;

   uint32_t left = separation;
   uint32_t top = separation;
   uint32_t maxHeight = 0;

   std::vector<GlyphInfo> glyphInfoVec;
   glyphInfoVec.reserve(atlasRunes.count());

   for (const auto rune : atlasRunes) {
      auto glyph = typeface.render_glyph(glyphSize, rune);
      assert(glyph.has_value());

      auto bottom = top + glyph->height;
      assert((bottom + separation) <= height);

      maxHeight = std::max(maxHeight, glyph->height + separation);
      auto right = left + glyph->width;
      if (right > width) {
         left = separation;
         right = left + glyph->width;
         top += maxHeight;
         bottom = top + glyph->height;
         maxHeight = glyph->height + separation;
      }

      m_glyphInfos.emplace(rune,
                           glyphInfoVec.emplace_back(GlyphInfo{{static_cast<float>(left) / widthFP, static_cast<float>(top) / heightFP},
                                                               {static_cast<float>(right) / widthFP, static_cast<float>(bottom) / heightFP},
                                                               {glyph->width, glyph->height},
                                                               {glyph->advanceX, glyph->advanceY},
                                                               {glyph->bitmapLeft, glyph->bitmapTop}}));

      for (u32 y = 0; y < glyph->height; ++y) {
         std::memcpy(&atlasData[left + (top + y) * width], &glyph->data[y * glyph->width], glyph->width);
      }

      left = right + separation;
   }

   m_texture.write(device, atlasData.data());
   m_texture.set_anisotropy_state(false);

   GAPI_CHECK_STATUS(m_glyphStorageBuffer.write_indirect(glyphInfoVec.data(), sizeof(GlyphInfo) * glyphInfoVec.size()));
}

std::vector<GlyphVertex> GlyphAtlas::create_glyph_vertices(const StringView text, TextMetric* outMetric) const
{
   std::vector<GlyphVertex> vertices;
   vertices.reserve(text.size() * 6);

   float x = 0.0f;
   float maxHeight = 0.0f;

   for (const auto ch : text) {
      if (not m_glyphInfos.contains(ch)) {
         x += m_glyphSize;
         continue;
      }

      const auto& info = m_glyphInfos.at(ch);
      if (maxHeight < info.size.y) {
         maxHeight = info.size.y;
      }

      const auto right = x + info.size.x;

      // top left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y}, glm::vec2{info.texCoordTopLeft.x, info.texCoordTopLeft.y});

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
      outMetric->width = x;
      outMetric->height = maxHeight;
   }

   return vertices;
}

const graphics_api::Texture& GlyphAtlas::texture() const
{
   return m_texture;
}

graphics_api::Texture& GlyphAtlas::texture()
{
   return m_texture;
}

TextMetric GlyphAtlas::measure_text(const StringView text) const
{
   float width = 0.0f;
   float height = 0.0f;

   for (const Rune rune : text) {
      if (not m_glyphInfos.contains(rune)) {
         width += m_glyphSize;
         continue;
      }

      const auto& info = m_glyphInfos.at(rune);
      if (info.size.y > height) {
         height = info.size.y;
      }

      width += info.advance.x;
   }

   return TextMetric{.width = width, .height = height};
}

u32 GlyphAtlas::find_rune_index(const StringView text, const float offset) const
{
   float width = 0.0f;

   u32 index = 0;
   for (const Rune rune : text) {
      float runeWidth;
      if (not m_glyphInfos.contains(rune)) {
         runeWidth = m_glyphSize;
      } else {
         const auto& info = m_glyphInfos.at(rune);
         runeWidth = info.advance.x;
      }

      if ((width + 0.5f * runeWidth) >= offset) {
         return index;
      }

      width += runeWidth;
      ++index;
   }

   return index;
}

const graphics_api::Buffer& GlyphAtlas::storage_buffer() const
{
   return m_glyphStorageBuffer;
}

}// namespace triglav::render_core