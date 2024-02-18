/*
TG_RESOURCE_TYPE(name, extension, cppType)
*/

namespace triglav::render_core {
struct Model;
struct Material;
}

namespace graphics_api {
class Texture;
class Shader;
}

namespace font {
class Typeface;
}

#define TG_RESOURCE_TYPE_LIST \
   TG_RESOURCE_TYPE(Texture, "tex", ::graphics_api::Texture) \
   TG_RESOURCE_TYPE(FragmentShader, "fshader", ::graphics_api::Shader) \
   TG_RESOURCE_TYPE(VertexShader, "vshader", ::graphics_api::Shader) \
   TG_RESOURCE_TYPE(Material, "mat", ::triglav::render_core::Material) \
   TG_RESOURCE_TYPE(Model, "model", ::triglav::render_core::Model) \
   TG_RESOURCE_TYPE(Typeface, "typeface", ::font::Typeface)
