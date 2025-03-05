#pragma once

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/io/Stream.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace triglav::gltf {

constexpr u32 g_invalidID = ~0;

struct Asset
{
   std::string generator;
   std::string version;
};

struct Scene
{
   std::vector<u32> nodes;
   std::optional<std::string> name;
};

struct Node
{
   std::vector<u32> children;
   std::optional<Matrix4x4> matrix;
   std::optional<u32> mesh;
   std::optional<Vector4> rotation;
   std::optional<Vector3> scale;
   std::optional<Vector3> translation;
   std::optional<std::string> name;
};

enum class PrimitiveAttributeType
{
   Position,
   Normal,
   Tangent,
   TexCoord,
   Color,
   Joints,
   Weights,
   Other,
};

struct PrimitiveAttribute
{
   PrimitiveAttributeType type;
   u32 accessorId;
};

struct Primitive
{
   std::vector<PrimitiveAttribute> attributes;
   std::optional<u32> indices;
   u32 mode;
   std::optional<u32> material;
};

struct Mesh
{
   std::vector<Primitive> primitives;
   std::string name;
};

enum class AccessorType
{
   Scalar,
   Vector2,
   Vector3,
   Vector4,
   Matrix2x2,
   Matrix3x3,
   Matrix4x4,
};

enum class ComponentType
{
   Byte,
   UnsignedByte,
   Short,
   UnsignedShort,
   UnsignedInt,
   Float,
};

struct Accessor
{
   u32 bufferView;
   u32 byteOffset;
   ComponentType componentType;
   u32 count;
   std::vector<float> max;
   std::vector<float> min;
   AccessorType type;
};

struct MaterialTexture
{
   u32 index;
};

struct PBRMetallicRoughness
{
   std::optional<MaterialTexture> baseColorTexture;
   std::optional<Vector4> baseColorFactor;
   float metallicFactor;
   float roughnessFactor;
};

struct Material
{
   PBRMetallicRoughness pbrMetallicRoughness;
   std::string name;
};

struct Texture
{
   u32 sampler;
   u32 source;
};

struct Image
{
   std::string uri;
};

struct Sampler
{
   u32 magFilter;
   u32 minFilter;
   u32 wrapS;
   u32 wrapT;
};

struct BufferView
{
   u32 buffer;
   u32 byteOffset;
   u32 byteLength;
   u32 target;
};

struct Buffer
{
   u32 byteLength;
   std::optional<std::string> uri;
};

struct Document
{
   Asset asset;
   u32 scene;
   std::vector<Scene> scenes;
   std::vector<Node> nodes;
   std::vector<Mesh> meshes;
   std::vector<Accessor> accessors;
   std::vector<Material> materials;
   std::vector<Texture> textures;
   std::vector<Image> images;
   std::vector<Sampler> samplers;
   std::vector<BufferView> bufferViews;
   std::vector<Buffer> buffers;

   void deserialize(io::IReader& reader);
};

}// namespace triglav::gltf