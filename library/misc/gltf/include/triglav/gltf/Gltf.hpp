#pragma once

#include "triglav/ArrayMap.hpp"
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
   Position = 0,
   Normal = 1,
   Tangent = 2,
   TexCoord = 32,
   Color = 2 * 32,
   Joints = 3 * 32,
   Weights = 4 * 32,
   Other = 5 * 32,
};

struct Primitive
{
   TG_META_STRUCT_BODY(Primitive)

   ArrayMap<PrimitiveAttributeType, u32> attributes;
   std::optional<u32> indices;
   u32 mode;
   std::optional<u32> material;
};

struct Mesh
{
   TG_META_STRUCT_BODY(Mesh)

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
   Byte = 5120,
   UnsignedByte = 5121,
   Short = 5122,
   UnsignedShort = 5123,
   UnsignedInt = 5125,
   Float = 5126,
};

struct Accessor
{
   TG_META_STRUCT_BODY(Accessor)

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
   TG_META_STRUCT_BODY(TextureInfo)

   u32 index;
};

struct PBRMetallicRoughness
{
   TG_META_STRUCT_BODY(PBRMetallicRoughness)

   std::optional<TextureInfo> base_color_texture;
   std::optional<Vector4> base_color_factor;
   float metallic_factor{1};
   float roughness_factor{1};
   std::optional<TextureInfo> metallic_roughness_texture;
};

struct Material
{
   TG_META_STRUCT_BODY(Material)

   PBRMetallicRoughness pbr_metallic_roughness;
   std::optional<TextureInfo> normal_texture;
   std::string name;
};

struct Texture
{
   TG_META_STRUCT_BODY(Texture)

   u32 sampler;
   u32 source;
   std::string name;
};

struct Image
{
   TG_META_STRUCT_BODY(Image)

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
   TG_META_STRUCT_BODY(Sampler)

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
   TG_META_STRUCT_BODY(BufferView)

   u32 buffer;
   u32 byte_offset = 0;
   u32 byte_length;
   std::optional<BufferTarget> target;
};

struct Buffer
{
   TG_META_STRUCT_BODY(Buffer)

   u32 byte_length;
   std::optional<std::string> uri;
};

struct Document
{
   TG_META_STRUCT_BODY(Document)

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