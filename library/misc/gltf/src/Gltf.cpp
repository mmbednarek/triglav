#include "Gltf.hpp"

#include "triglav/io/BufferedReader.hpp"
#include "triglav/json_util/JsonUtil.hpp"

#include <array>

namespace triglav::gltf {

namespace {

Asset deserialize_asset(const rapidjson::Value& value)
{
   Asset asset{};
   asset.generator = value["generator"].GetString();
   asset.version = value["version"].GetString();
   return asset;
}

Scene deserialize_scene(const rapidjson::Value& value)
{
   Scene scene{};
   for (const auto& child : value["nodes"].GetArray()) {
      scene.nodes.push_back(child.GetInt());
   }
   if (value.HasMember("name")) {
      scene.name.emplace(value["name"].GetString());
   }
   return scene;
}

Vector3 deserialize_vector3(const rapidjson::Value& value)
{
   std::array<float, 3> vec_data{};
   auto it = vec_data.begin();
   for (const auto& child : value.GetArray()) {
      *it = child.GetFloat();
      ++it;
   }

   Vector3 result{};
   std::memcpy(reinterpret_cast<float*>(&result), vec_data.data(), sizeof(float) * 3);

   return result;
}

Vector4 deserialize_vector4(const rapidjson::Value& value)
{
   std::array<float, 4> vec_data{};
   auto it = vec_data.begin();
   for (const auto& child : value.GetArray()) {
      *it = child.GetFloat();
      ++it;
   }

   Vector4 result{};
   std::memcpy(reinterpret_cast<float*>(&result), vec_data.data(), sizeof(float) * 4);

   return result;
}

Matrix4x4 deserialize_matrix4x4(const rapidjson::Value& value)
{
   std::array<float, 16> mat_data{};
   auto it = mat_data.begin();
   for (const auto& child : value.GetArray()) {
      *it = child.GetFloat();
      ++it;
   }

   Matrix4x4 result{};
   std::memcpy(reinterpret_cast<float*>(&result), mat_data.data(), sizeof(float) * 16);

   return result;
}

Node deserialize_node(const rapidjson::Value& value)
{
   Node scene{};
   if (value.HasMember("children")) {
      for (const auto& child : value["children"].GetArray()) {
         scene.children.push_back(child.GetInt());
      }
   }
   if (value.HasMember("matrix")) {
      scene.matrix.emplace(deserialize_matrix4x4(value["matrix"]));
   }
   if (value.HasMember("mesh")) {
      scene.mesh.emplace(value["mesh"].GetInt());
   }
   if (value.HasMember("rotation")) {
      scene.rotation.emplace(deserialize_vector4(value["rotation"]));
   }
   if (value.HasMember("scale")) {
      scene.scale.emplace(deserialize_vector3(value["scale"]));
   }
   if (value.HasMember("translation")) {
      scene.translation.emplace(deserialize_vector3(value["translation"]));
   }
   if (value.HasMember("name")) {
      scene.name.emplace(value["name"].GetString());
   }
   return scene;
}

PrimitiveAttributeType primitive_attribute_type_from_string(const std::string_view value)
{
   if (value == "POSITION")
      return PrimitiveAttributeType::Position;
   if (value == "NORMAL")
      return PrimitiveAttributeType::Normal;
   if (value == "TANGENT")
      return PrimitiveAttributeType::Tangent;
   if (value.starts_with("TEXCOORD")) {
      return PrimitiveAttributeType::TexCoord;
   }
   if (value.starts_with("COLOR")) {
      return PrimitiveAttributeType::Color;
   }
   if (value.starts_with("JOINTS")) {
      return PrimitiveAttributeType::Joints;
   }
   if (value.starts_with("WEIGHTS")) {
      return PrimitiveAttributeType::Weights;
   }

   return PrimitiveAttributeType::Other;
}

Primitive deserialize_primitive(const rapidjson::Value& value)
{
   Primitive primitive{};

   for (const auto& member : value["attributes"].GetObject()) {
      primitive.attributes.emplace_back(primitive_attribute_type_from_string(member.name.GetString()), member.value.GetInt());
   }
   if (value.HasMember("indices")) {
      primitive.indices.emplace(value["indices"].GetInt());
   }
   if (value.HasMember("mode")) {
      primitive.mode = value["mode"].GetInt();
   } else {
      primitive.mode = 4;
   }
   if (value.HasMember("material")) {
      primitive.material.emplace(value["material"].GetInt());
   }

   return primitive;
}

Mesh deserialize_mesh(const rapidjson::Value& value)
{
   Mesh mesh{};

   for (const auto& child : value["primitives"].GetArray()) {
      mesh.primitives.push_back(deserialize_primitive(child));
   }
   mesh.name = value["name"].GetString();

   return mesh;
}

AccessorType accessor_type_from_string(const std::string_view value)
{
   using namespace std::string_view_literals;

   if (value == "SCALAR"sv)
      return AccessorType::Scalar;
   if (value == "VEC2"sv)
      return AccessorType::Vector2;
   if (value == "VEC3"sv)
      return AccessorType::Vector3;
   if (value == "VEC4"sv)
      return AccessorType::Vector4;
   if (value == "MAT2"sv)
      return AccessorType::Matrix2x2;
   if (value == "MAT3"sv)
      return AccessorType::Matrix3x3;
   if (value == "MAT4"sv)
      return AccessorType::Matrix4x4;

   assert(false);
   return AccessorType::Scalar;
}

ComponentType component_type_from_int(const int value)
{
   switch (value) {
   case 5120:
      return ComponentType::Byte;
   case 5121:
      return ComponentType::UnsignedByte;
   case 5122:
      return ComponentType::Short;
   case 5123:
      return ComponentType::UnsignedShort;
   case 5125:
      return ComponentType::UnsignedInt;
   case 5126:
      return ComponentType::Float;
   default:
      break;
   }

   assert(false);
   return ComponentType::Byte;
}

Accessor deserialize_accessor(const rapidjson::Value& value)
{
   Accessor accessor{};

   accessor.buffer_view = value["bufferView"].GetInt();
   if (value.HasMember("byteOffset")) {
      accessor.byte_offset = value["byteOffset"].GetInt();
   } else {
      accessor.byte_offset = 0;
   }
   accessor.component_type = component_type_from_int(value["componentType"].GetInt());
   accessor.count = value["count"].GetInt();

   if (value.HasMember("max")) {
      for (const auto& child : value["max"].GetArray()) {
         accessor.max.push_back(child.GetFloat());
      }
   }
   if (value.HasMember("min")) {
      for (const auto& child : value["min"].GetArray()) {
         accessor.min.push_back(child.GetFloat());
      }
   }
   accessor.type = accessor_type_from_string(value["type"].GetString());

   return accessor;
}

MaterialTexture deserialize_material_texture(const rapidjson::Value& value)
{
   MaterialTexture texture{};
   texture.index = value["index"].GetInt();
   return texture;
}

NormalMapTexture deserialize_normal_map_texture(const rapidjson::Value& value)
{
   NormalMapTexture texture{};
   texture.index = value["index"].GetInt();
   return texture;
}

PBRMetallicRoughness deserialize_pbr_metallic_roughness(const rapidjson::Value& value)
{
   PBRMetallicRoughness pbr_metallic_roughness{};
   if (value.HasMember("baseColorTexture")) {
      pbr_metallic_roughness.base_color_texture.emplace(deserialize_material_texture(value["baseColorTexture"]));
   }
   if (value.HasMember("baseColorFactor")) {
      pbr_metallic_roughness.base_color_factor.emplace(deserialize_vector4(value["baseColorFactor"]));
   }
   if (value.HasMember("metallicFactor")) {
      pbr_metallic_roughness.metallic_factor = value["metallicFactor"].GetFloat();
   }
   if (value.HasMember("roughnessFactor")) {
      pbr_metallic_roughness.roughness_factor = value["roughnessFactor"].GetFloat();
   }
   return pbr_metallic_roughness;
}

Material deserialize_material(const rapidjson::Value& value)
{
   Material material{};
   material.name = value["name"].GetString();
   material.pbr_metallic_roughness = deserialize_pbr_metallic_roughness(value["pbrMetallicRoughness"]);
   if (value.HasMember("normalTexture")) {
      material.normal_texture.emplace(deserialize_normal_map_texture(value["normalTexture"]));
   }
   return material;
}

Texture deserialize_texture(const rapidjson::Value& value)
{
   Texture texture{};
   texture.sampler = value["sampler"].GetInt();
   texture.source = value["source"].GetInt();
   if (value.HasMember("name")) {
      texture.name = value["name"].GetString();
   }
   return texture;
}

Image deserialize_image(const rapidjson::Value& value)
{
   Image image{};
   if (value.HasMember("uri")) {
      image.uri = value["uri"].GetString();
   }
   if (value.HasMember("mimeType")) {
      image.mime_type = value["mimeType"].GetString();
   }
   if (value.HasMember("bufferView")) {
      image.buffer_view = value["bufferView"].GetInt();
   }
   if (value.HasMember("name")) {
      image.name = value["name"].GetString();
   }
   return image;
}

Sampler deserialize_sampler(const rapidjson::Value& value)
{
   Sampler sampler{};
   sampler.mag_filter = static_cast<SamplerFilter>(value["magFilter"].GetInt());
   sampler.min_filter = static_cast<SamplerFilter>(value["minFilter"].GetInt());
   if (value.HasMember("wrapS")) {
      sampler.wrap_s = static_cast<SamplerWrap>(value["wrapS"].GetInt());
   }
   if (value.HasMember("wrapT")) {
      sampler.wrap_t = static_cast<SamplerWrap>(value["wrapT"].GetInt());
   }
   if (value.HasMember("name")) {
      sampler.name = value["name"].GetString();
   }
   return sampler;
}

BufferView deserialize_buffer_view(const rapidjson::Value& value)
{
   BufferView buffer_view{};
   buffer_view.buffer = value["buffer"].GetInt();
   if (value.HasMember("byteOffset")) {
      buffer_view.byte_offset = value["byteOffset"].GetInt();
   }
   buffer_view.byte_length = value["byteLength"].GetInt();
   if (value.HasMember("target")) {
      buffer_view.target = static_cast<BufferTarget>(value["target"].GetInt());
   }
   return buffer_view;
}

Buffer deserialize_buffer(const rapidjson::Value& value)
{
   Buffer buffer{};
   if (value.HasMember("uri")) {
      buffer.uri.emplace(value["uri"].GetString());
   }
   buffer.byte_length = value["byteLength"].GetInt();
   return buffer;
}

}// namespace

void Document::deserialize(io::IReader& reader)
{
   json_util::RapidJsonInputStream stream(reader);

   rapidjson::Document doc;
   doc.ParseStream(stream);

   this->asset = deserialize_asset(doc["asset"]);
   this->scene = doc["scene"].GetInt();

   for (const auto& child : doc["scenes"].GetArray()) {
      this->scenes.push_back(deserialize_scene(child));
   }
   for (const auto& child : doc["nodes"].GetArray()) {
      this->nodes.push_back(deserialize_node(child));
   }
   for (const auto& child : doc["meshes"].GetArray()) {
      this->meshes.push_back(deserialize_mesh(child));
   }
   for (const auto& child : doc["accessors"].GetArray()) {
      this->accessors.push_back(deserialize_accessor(child));
   }
   if (doc.HasMember("materials")) {
      for (const auto& child : doc["materials"].GetArray()) {
         this->materials.push_back(deserialize_material(child));
      }
   }
   if (doc.HasMember("textures")) {
      for (const auto& child : doc["textures"].GetArray()) {
         this->textures.push_back(deserialize_texture(child));
      }
   }
   if (doc.HasMember("images")) {
      for (const auto& child : doc["images"].GetArray()) {
         this->images.push_back(deserialize_image(child));
      }
   }
   if (doc.HasMember("samplers")) {
      for (const auto& child : doc["samplers"].GetArray()) {
         this->samplers.push_back(deserialize_sampler(child));
      }
   }
   for (const auto& child : doc["bufferViews"].GetArray()) {
      this->buffer_views.push_back(deserialize_buffer_view(child));
   }
   for (const auto& child : doc["buffers"].GetArray()) {
      this->buffers.push_back(deserialize_buffer(child));
   }
}

}// namespace triglav::gltf
