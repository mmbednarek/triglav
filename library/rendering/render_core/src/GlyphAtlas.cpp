#include "GlyphAtlas.hpp"

#include "RenderCore.hpp"

#include "triglav/graphics_api/Device.hpp"

#include <cstring>
#include <locale>
#include <vector>

namespace triglav::render_core {

namespace gapi = graphics_api;

GlyphAtlas::GlyphAtlas(gapi::Device& device, const font::Typeface& typeface, const font::Charset& atlas_runes, const int glyph_size,
                       const uint32_t width, const uint32_t height) :
    m_glyph_size(static_cast<float>(glyph_size)),
    m_texture(check_result(device.create_texture(GAPI_FORMAT(R, UNorm8), {width, height}))),
    m_glyph_storage_buffer(
       GAPI_CHECK(device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::TransferDst | gapi::BufferUsage::TransferSrc,
                                       sizeof(GlyphInfo) * atlas_runes.count())))
{
   std::vector<uint8_t> atlas_data{};
   atlas_data.resize(width * height);

   const auto width_fp = static_cast<float>(width);
   const auto height_fp = static_cast<float>(height);

   static constexpr uint32_t separation = 4;

   uint32_t left = separation;
   uint32_t top = separation;
   uint32_t max_height = 0;

   std::vector<GlyphInfo> glyph_info_vec;
   glyph_info_vec.reserve(atlas_runes.count());

   for (const auto rune : atlas_runes) {
      auto glyph = typeface.render_glyph(glyph_size, rune);
      assert(glyph.has_value());

      auto bottom = top + glyph->height;
      assert((bottom + separation) <= height);

      max_height = std::max(max_height, glyph->height + separation);
      auto right = left + glyph->width;
      if (right > width) {
         left = separation;
         right = left + glyph->width;
         top += max_height;
         bottom = top + glyph->height;
         max_height = glyph->height + separation;
      }

      m_glyph_infos.emplace(
         rune, glyph_info_vec.emplace_back(GlyphInfo{{static_cast<float>(left) / width_fp, static_cast<float>(top) / height_fp},
                                                     {static_cast<float>(right) / width_fp, static_cast<float>(bottom) / height_fp},
                                                     {glyph->width, glyph->height},
                                                     {glyph->advance_x, glyph->advance_y},
                                                     {glyph->bitmap_left, glyph->bitmap_top}}));

      for (u32 y = 0; y < glyph->height; ++y) {
         std::memcpy(&atlas_data[left + (top + y) * width], &glyph->data[y * glyph->width], glyph->width);
      }

      left = right + separation;
   }

   m_texture.write(device, atlas_data.data());
   m_texture.set_anisotropy_state(false);

   GAPI_CHECK_STATUS(m_glyph_storage_buffer.write_indirect(glyph_info_vec.data(), sizeof(GlyphInfo) * glyph_info_vec.size()));
}

std::vector<GlyphVertex> GlyphAtlas::create_glyph_vertices(const StringView text, TextMetric* out_metric) const
{
   std::vector<GlyphVertex> vertices;
   vertices.reserve(text.size() * 6);

   float x = 0.0f;
   float max_height = 0.0f;

   for (const auto ch : text) {
      if (not m_glyph_infos.contains(ch)) {
         x += m_glyph_size;
         continue;
      }

      const auto& info = m_glyph_infos.at(ch);
      if (max_height < info.size.y) {
         max_height = info.size.y;
      }

      const auto right = x + info.size.x;

      // top left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y},
                            glm::vec2{info.tex_coord_top_left.x, info.tex_coord_top_left.y});

      // bottom left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.tex_coord_top_left.x, info.tex_coord_bottom_right.y});

      // top right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y},
                            glm::vec2{info.tex_coord_bottom_right.x, info.tex_coord_top_left.y});

      // top right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y},
                            glm::vec2{info.tex_coord_bottom_right.x, info.tex_coord_top_left.y});

      // bottom left
      vertices.emplace_back(glm::vec2{x + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.tex_coord_top_left.x, info.tex_coord_bottom_right.y});

      // bottom right
      vertices.emplace_back(glm::vec2{right + info.padding.x, -info.padding.y + info.size.y},
                            glm::vec2{info.tex_coord_bottom_right.x, info.tex_coord_bottom_right.y});

      x += info.advance.x;
   }

   if (out_metric != nullptr) {
      out_metric->width = x;
      out_metric->height = max_height;
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
      if (not m_glyph_infos.contains(rune)) {
         width += m_glyph_size;
         continue;
      }

      const auto& info = m_glyph_infos.at(rune);
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
      float rune_width;
      if (not m_glyph_infos.contains(rune)) {
         rune_width = m_glyph_size;
      } else {
         const auto& info = m_glyph_infos.at(rune);
         rune_width = info.advance.x;
      }

      if ((width + 0.5f * rune_width) >= offset) {
         return index;
      }

      width += rune_width;
      ++index;
   }

   return index;
}

const graphics_api::Buffer& GlyphAtlas::storage_buffer() const
{
   return m_glyph_storage_buffer;
}

}// namespace triglav::render_core