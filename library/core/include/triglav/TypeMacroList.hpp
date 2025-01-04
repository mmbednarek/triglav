namespace triglav {
namespace render_core {
class GlyphAtlas;
}// namespace render_core

namespace render_objects {
struct Material;
struct MaterialTemplate;
struct Model;
}// namespace render_objects

namespace graphics_api {
class Texture;
class Shader;
}// namespace graphics_api

namespace font {
class Typeface;
}

namespace world {
class Level;
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
   TG_RESOURCE_TYPE(Material, "mat", ::triglav::render_objects::Material, 1)                \
   TG_RESOURCE_TYPE(MaterialTemplate, "mt", ::triglav::render_objects::MaterialTemplate, 0) \
   TG_RESOURCE_TYPE(Model, "model", ::triglav::render_objects::Model, 0)                    \
   TG_RESOURCE_TYPE(Typeface, "typeface", ::triglav::font::Typeface, 0)                     \
   TG_RESOURCE_TYPE(Level, "level", ::triglav::world::Level, 0)
