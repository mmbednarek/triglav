#pragma once

#include "triglav/Name.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <variant>
#include <vector>

namespace triglav::render_objects {

enum class MaterialPropertyType
{
   Texture2D,
   Float32,
   Vector3,
   Vector4,
   Matrix4x4
};

enum class PropertySource
{
   Constant,
   LastFrameColorOut,
   LastFrameDepthOut,
   LastViewProjectionMatrix,
   ViewPosition,
};

struct MaterialProperty
{
   Name name;
   MaterialPropertyType type;
   PropertySource source;
};

struct MaterialTemplate
{
   FragmentShaderName fragmentShader;
   VertexShaderName vertexShader;
   std::vector<MaterialProperty> properties;
};

using MaterialPropertyValue = std::variant<TextureName, float, glm::vec3, glm::vec4, glm::mat4>;

struct Material
{
   MaterialTemplateName materialTemplate;
   std::vector<MaterialPropertyValue> values{};
};

}// namespace triglav::render_objects