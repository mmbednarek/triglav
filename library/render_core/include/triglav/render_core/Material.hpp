#pragma once

#include "triglav/Name.hpp"

#include <vector>
#include <variant>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace triglav::render_core {

enum class MaterialPropertyType {
   Texture2D,
   Float32,
   Vec3,
   Vec4
};

struct MaterialProperty {
   Name name;
   MaterialPropertyType type;
};

struct MaterialTemplate {
   FragmentShaderName fragmentShader;
   VertexShaderName vertexShader;
   std::vector<MaterialProperty> properties;
};

using MaterialPropertyValue = std::variant<TextureName, float, glm::vec3, glm::vec4>;

struct Material {
   MaterialTemplateName materialTemplate;
   std::vector<MaterialPropertyValue> values{};
};

}// namespace renderer