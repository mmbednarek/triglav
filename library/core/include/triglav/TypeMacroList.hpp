/*
TG_RESOURCE_TYPE(name, extension, cppType)
*/

namespace triglav {
namespace render_core {
struct GlyphAtlas;
struct Model;
struct Material;
}// namespace render_core

namespace graphics_api {
class Texture;
class Shader;
class Sampler;
}// namespace graphics_api

namespace font {
class Typeface;
}

namespace world {
struct Level;
}// namespace world

}// namespace triglav

#define TG_RESOURCE_TYPE_LIST                                                   \
   TG_RESOURCE_TYPE(Texture, "tex", ::triglav::graphics_api::Texture)           \
   TG_RESOURCE_TYPE(FragmentShader, "fshader", ::triglav::graphics_api::Shader) \
   TG_RESOURCE_TYPE(VertexShader, "vshader", ::triglav::graphics_api::Shader)   \
   TG_RESOURCE_TYPE(Sampler, "sampler", ::triglav::graphics_api::Sampler)       \
   TG_RESOURCE_TYPE(Material, "mat", ::triglav::render_core::Material)          \
   TG_RESOURCE_TYPE(Model, "model", ::triglav::render_core::Model)              \
   TG_RESOURCE_TYPE(GlyphAtlas, "glyphs", ::triglav::render_core::GlyphAtlas)   \
   TG_RESOURCE_TYPE(Typeface, "typeface", ::triglav::font::Typeface)            \
   TG_RESOURCE_TYPE(Level, "level", ::triglav::world::Level)
