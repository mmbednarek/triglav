#include "Gltf.hpp"

#include "triglav/io/BufferedReader.hpp"

#include <array>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <rapidjson/document.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

namespace triglav::gltf {

namespace {

class RapidJsonInputStream
{
 public:
   using Ch = char;

   explicit RapidJsonInputStream(io::IReader& reader) :
       m_reader(reader)
   {
   }

   Ch Peek() const
   {
      if (!m_reader.has_next())
         return 0;

      return m_reader.peek();
   }

   Ch Take()
   {
      if (!m_reader.has_next())
         return 0;

      m_totalBytesRead += 1;
      return m_reader.next();
   }

   size_t Tell()
   {
      return m_totalBytesRead;
   }

   Ch* PutBegin()
   {
      assert(false);
   }

   void Put(Ch /*c*/)
   {
      assert(false);
   }

   void Flush() {}

   size_t PutEnd(Ch* /*begin*/)
   {
      assert(false);
   }

 private:
   mutable io::BufferedReader m_reader;
   size_t m_totalBytesRead = 0;
};

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
   return scene;
}

Vector4 deserialize_vector4(const rapidjson::Value& value)
{
   std::array<float, 4> vecData{};
   auto it = vecData.begin();
   for (const auto& child : value.GetArray()) {
      *it = child.GetFloat();
      ++it;
   }

   Vector4 result{};
   std::memcpy(reinterpret_cast<float*>(&result), vecData.data(), sizeof(float) * 4);

   return result;
}

Matrix4x4 deserialize_matrix4x4(const rapidjson::Value& value)
{
   std::array<float, 16> matData{};
   auto it = matData.begin();
   for (const auto& child : value.GetArray()) {
      *it = child.GetFloat();
      ++it;
   }

   Matrix4x4 result{};
   std::memcpy(reinterpret_cast<float*>(&result), matData.data(), sizeof(float) * 16);

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
}

Accessor deserialize_accessor(const rapidjson::Value& value)
{
   Accessor accessor{};

   accessor.bufferView = value["bufferView"].GetInt();
   if (value.HasMember("byteOffset")) {
      accessor.byteOffset = value["byteOffset"].GetInt();
   } else {
      accessor.byteOffset = 0;
   }
   accessor.componentType = component_type_from_int(value["componentType"].GetInt());
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

PBRMetallicRoughness deserialize_pbr_metallic_roughness(const rapidjson::Value& value)
{
   PBRMetallicRoughness pbrMetallicRoughness{};
   if (value.HasMember("baseColorTexture")) {
      pbrMetallicRoughness.baseColorTexture.emplace(deserialize_material_texture(value["baseColorTexture"]));
   }
   if (value.HasMember("baseColorFactor")) {
      pbrMetallicRoughness.baseColorFactor.emplace(deserialize_vector4(value["baseColorFactor"]));
   }
   if (value.HasMember("metallicFactor")) {
      pbrMetallicRoughness.metallicFactor = value["metallicFactor"].GetFloat();
   }
   if (value.HasMember("roughnessFactor")) {
      pbrMetallicRoughness.roughnessFactor = value["roughnessFactor"].GetFloat();
   }
   return pbrMetallicRoughness;
}

Material deserialize_material(const rapidjson::Value& value)
{
   Material material{};
   material.name = value["name"].GetString();
   material.pbrMetallicRoughness = deserialize_pbr_metallic_roughness(value["pbrMetallicRoughness"]);
   return material;
}

Texture deserialize_texture(const rapidjson::Value& value)
{
   Texture texture{};
   texture.sampler = value["sampler"].GetInt();
   texture.source = value["source"].GetInt();
   return texture;
}

Image deserialize_image(const rapidjson::Value& value)
{
   Image image{};
   image.uri = value["uri"].GetString();
   return image;
}

Sampler deserialize_sampler(const rapidjson::Value& value)
{
   Sampler sampler{};
   sampler.magFilter = value["magFilter"].GetInt();
   sampler.minFilter = value["minFilter"].GetInt();
   sampler.wrapS = value["wrapS"].GetInt();
   sampler.wrapT = value["wrapT"].GetInt();
   return sampler;
}

BufferView deserialize_buffer_view(const rapidjson::Value& value)
{
   BufferView bufferView{};
   bufferView.buffer = value["buffer"].GetInt();
   bufferView.byteOffset = value["byteOffset"].GetInt();
   bufferView.byteLength = value["byteLength"].GetInt();
   bufferView.target = value["target"].GetInt();
   return bufferView;
}

Buffer deserialize_buffer(const rapidjson::Value& value)
{
   Buffer buffer{};
   if (value.HasMember("uri")) {
      buffer.uri.emplace(value["uri"].GetString());
   }
   buffer.byteLength = value["byteLength"].GetInt();
   return buffer;
}

}// namespace

void Document::deserialize(io::IReader& reader)
{
   RapidJsonInputStream stream(reader);

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
      this->bufferViews.push_back(deserialize_buffer_view(child));
   }
   for (const auto& child : doc["buffers"].GetArray()) {
      this->buffers.push_back(deserialize_buffer(child));
   }
}

}// namespace triglav::gltf
