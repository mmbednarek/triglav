namespace triglav {
namespace render_core {
class GlyphAtlas;
struct Model;
struct Material;
struct MaterialTemplate;
}// namespace render_core

namespace graphics_api {
class Texture;
class Shader;
}// namespace graphics_api

namespace font {
class Typeface;
}

namespace world {
struct Level;
}// namespace world

}// namespace triglav

/*
TG_RESOURCE_TYPE(name, extension, cppType, loadingStage)
*/

#define TG_RESOURCE_TYPE_LIST                                                               \
   TG_RESOURCE_TYPE(Texture, "tex", ::triglav::graphics_api::Texture, 0)                    \
   TG_RESOURCE_TYPE(FragmentShader, "fshader", ::triglav::graphics_api::Shader, 0)          \
   TG_RESOURCE_TYPE(VertexShader, "vshader", ::triglav::graphics_api::Shader, 0)            \
   TG_RESOURCE_TYPE(ComputeShader, "cshader", ::triglav::graphics_api::Shader, 0)           \
   TG_RESOURCE_TYPE(RayGenShader, "rgenshader", ::triglav::graphics_api::Shader, 0)         \
   TG_RESOURCE_TYPE(RayClosestHitShader, "rchitshader", ::triglav::graphics_api::Shader, 0) \
   TG_RESOURCE_TYPE(RayMissShader, "rmissshader", ::triglav::graphics_api::Shader, 0)       \
   TG_RESOURCE_TYPE(Material, "mat", ::triglav::render_core::Material, 1)                   \
   TG_RESOURCE_TYPE(MaterialTemplate, "mt", ::triglav::render_core::MaterialTemplate, 0)    \
   TG_RESOURCE_TYPE(Model, "model", ::triglav::render_core::Model, 0)                       \
   TG_RESOURCE_TYPE(Typeface, "typeface", ::triglav::font::Typeface, 0)                     \
   TG_RESOURCE_TYPE(Level, "level", ::triglav::world::Level, 0)
