#pragma once

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/io/Stream.hpp"
#include "triglav/meta/Meta.hpp"

#include <optional>
#include <string>
#include <vector>

namespace triglav::gltf {

constexpr u32 g_invalid_id = ~0u;

struct Asset
{
   TG_META_STRUCT_BODY(Asset)
   std::string generator;
   std::string version;
};

struct Scene
{
   TG_META_STRUCT_BODY(Scene)
   std::vector<u32> nodes;
   std::optional<std::string> name;
};

struct Node
{
   TG_META_STRUCT_BODY(Node)
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
   u32 accessor_id;
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
   u32 buffer_view;
   u32 byte_offset;
   ComponentType component_type;
   u32 count;
   std::vector<float> max;
   std::vector<float> min;
   AccessorType type;
};

struct TextureInfo
{
   u32 index;
};

struct PBRMetallicRoughness
{
   std::optional<TextureInfo> base_color_texture;
   std::optional<Vector4> base_color_factor;
   float metallic_factor{1};
   float roughness_factor{1};
   std::optional<TextureInfo> metallic_roughness_texture;
};

struct Material
{
   PBRMetallicRoughness pbr_metallic_roughness;
   std::optional<TextureInfo> normal_texture;
   std::string name;
};

struct Texture
{
   u32 sampler;
   u32 source;
   std::string name;
};

struct Image
{
   std::optional<std::string> uri;
   std::optional<std::string> mime_type;
   std::optional<u32> buffer_view;
   std::string name;
};

enum class SamplerFilter : u32
{
   Nearest = 9728,
   Linear = 9729,
   NearestMipmapNearest = 9984,
   LinearMipmapNearest = 9985,
   NearestMipmapLinear = 9986,
   LinearMipmapLinear = 9987,
};

enum class SamplerWrap : u32
{
   ClampToEdge = 33071,
   MirroredRepeat = 33648,
   Repeat = 10497,
};

struct Sampler
{
   SamplerFilter mag_filter;
   SamplerFilter min_filter;
   SamplerWrap wrap_s = SamplerWrap::Repeat;
   SamplerWrap wrap_t = SamplerWrap::Repeat;
   std::string name;
};

enum class BufferTarget
{
   ArrayBuffer = 34962,
   ElementArrayBuffer = 34963,
};

struct BufferView
{
   u32 buffer;
   u32 byte_offset = 0;
   u32 byte_length;
   std::optional<BufferTarget> target;
};

struct Buffer
{
   u32 byte_length;
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
   std::vector<BufferView> buffer_views;
   std::vector<Buffer> buffers;

   void deserialize(io::IReader& reader);
};

}// namespace triglav::gltf