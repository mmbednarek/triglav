#include "Gltf.hpp"

#include "triglav/io/BufferedReader.hpp"
#include "triglav/json_util/Deserialize.hpp"
#include "triglav/json_util/JsonUtil.hpp"

#include <array>

namespace triglav::gltf {

void Document::deserialize(io::IReader& reader)
{
   json_util::deserialize(this->to_meta_ref(), reader);
}

}// namespace triglav::gltf

#define TG_TYPE(NS) NS(NS(triglav, gltf), Asset)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(generator, std::string)
TG_META_PROPERTY(version, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Scene)
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(nodes, triglav::u32)
TG_META_OPTIONAL_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Node)
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(children, triglav::u32)
TG_META_OPTIONAL_PROPERTY(matrix, triglav::Matrix4x4)
TG_META_OPTIONAL_PROPERTY(mesh, triglav::u32)
TG_META_OPTIONAL_PROPERTY(rotation, triglav::Vector4)
TG_META_OPTIONAL_PROPERTY(scale, triglav::Vector3)
TG_META_OPTIONAL_PROPERTY(translation, triglav::Vector3)
TG_META_OPTIONAL_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), PrimitiveAttributeType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE_STR(Position, "POSITION")
TG_META_ENUM_VALUE_STR(Normal, "NORMAL")
TG_META_ENUM_VALUE_STR(Tangent, "TANGENT")
TG_META_ENUM_VALUE_STR(TexCoord, "TEXCOORD")
TG_META_ENUM_VALUE_STR(Color, "COLOR")
TG_META_ENUM_VALUE_STR(Joints, "JOINTS")
TG_META_ENUM_VALUE_STR(Weights, "WEIGHTS")
TG_META_ENUM_VALUE_STR(Other, "OTHER")
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Primitive)
TG_META_CLASS_BEGIN
TG_META_MAP_PROPERTY(attributes, triglav::gltf::PrimitiveAttributeType, triglav::u32)
TG_META_OPTIONAL_PROPERTY(indices, triglav::u32)
TG_META_PROPERTY(mode, triglav::u32)
TG_META_OPTIONAL_PROPERTY(material, triglav::u32)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Mesh)
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(primitives, triglav::gltf::Primitive)
TG_META_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), AccessorType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE_STR(Scalar, "SCALAR")
TG_META_ENUM_VALUE_STR(Vector2, "VEC2")
TG_META_ENUM_VALUE_STR(Vector3, "VEC3")
TG_META_ENUM_VALUE_STR(Vector4, "VEC4")
TG_META_ENUM_VALUE_STR(Matrix2x2, "MAT2")
TG_META_ENUM_VALUE_STR(Matrix3x3, "MAT3")
TG_META_ENUM_VALUE_STR(Matrix4x4, "MAT4")
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), ComponentType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE_STR(Byte, "BYTE")
TG_META_ENUM_VALUE_STR(UnsignedByte, "UNSIGNED_BYTE")
TG_META_ENUM_VALUE_STR(Short, "SHORT")
TG_META_ENUM_VALUE_STR(UnsignedShort, "UNSIGNED_SHORT")
TG_META_ENUM_VALUE_STR(UnsignedInt, "UNSIGNED_INT")
TG_META_ENUM_VALUE_STR(Float, "FLOAT")
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Accessor)
TG_META_CLASS_BEGIN
TG_META_PROPERTY_STR(buffer_view, "bufferView", triglav::u32)
TG_META_PROPERTY_STR(byte_offset, "byteOffset", triglav::u32)
TG_META_PROPERTY_STR(component_type, "componentType", triglav::gltf::ComponentType)
TG_META_PROPERTY(count, triglav::u32)
TG_META_ARRAY_PROPERTY(max, float)
TG_META_ARRAY_PROPERTY(min, float)
TG_META_PROPERTY(type, triglav::gltf::AccessorType)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), TextureInfo)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(index, triglav::u32)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), PBRMetallicRoughness)
TG_META_CLASS_BEGIN
TG_META_OPTIONAL_PROPERTY_STR(base_color_texture, "baseColorTexture", triglav::gltf::TextureInfo)
TG_META_OPTIONAL_PROPERTY_STR(base_color_factor, "baseColorFactor", triglav::Vector4)
TG_META_PROPERTY_STR(metallic_factor, "metallicFactor", float)
TG_META_PROPERTY_STR(roughness_factor, "roughnessFactor", float)
TG_META_OPTIONAL_PROPERTY_STR(metallic_roughness_texture, "metallicRoughnessTexture", triglav::gltf::TextureInfo)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Material)
TG_META_CLASS_BEGIN
TG_META_PROPERTY_STR(pbr_metallic_roughness, "pbrMetallicRoughness", triglav::gltf::PBRMetallicRoughness)
TG_META_OPTIONAL_PROPERTY_STR(normal_texture, "normalTexture", triglav::gltf::TextureInfo)
TG_META_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Texture)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(sampler, triglav::u32)
TG_META_PROPERTY(source, triglav::u32)
TG_META_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Image)
TG_META_CLASS_BEGIN
TG_META_OPTIONAL_PROPERTY(uri, std::string)
TG_META_OPTIONAL_PROPERTY_STR(mime_type, "mimeType", std::string)
TG_META_OPTIONAL_PROPERTY_STR(buffer_view, "bufferView", triglav::u32)
TG_META_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), SamplerFilter)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Nearest)
TG_META_ENUM_VALUE(Linear)
TG_META_ENUM_VALUE(NearestMipmapNearest)
TG_META_ENUM_VALUE(LinearMipmapNearest)
TG_META_ENUM_VALUE(NearestMipmapLinear)
TG_META_ENUM_VALUE(LinearMipmapLinear)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), SamplerWrap)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(ClampToEdge)
TG_META_ENUM_VALUE(MirroredRepeat)
TG_META_ENUM_VALUE(Repeat)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Sampler)
TG_META_CLASS_BEGIN
TG_META_PROPERTY_STR(mag_filter, "magFilter", triglav::gltf::SamplerFilter)
TG_META_PROPERTY_STR(min_filter, "minFilter", triglav::gltf::SamplerFilter)
TG_META_PROPERTY_STR(wrap_s, "wrapS", triglav::gltf::SamplerWrap)
TG_META_PROPERTY_STR(wrap_t, "wrapT", triglav::gltf::SamplerWrap)
TG_META_PROPERTY(name, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), BufferTarget)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(ArrayBuffer)
TG_META_ENUM_VALUE(ElementArrayBuffer)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), BufferView)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(buffer, triglav::u32)
TG_META_PROPERTY_STR(byte_offset, "byteOffset", triglav::u32)
TG_META_PROPERTY_STR(byte_length, "byteLength", triglav::u32)
TG_META_OPTIONAL_PROPERTY(target, triglav::gltf::BufferTarget)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Buffer)
TG_META_CLASS_BEGIN
TG_META_PROPERTY_STR(byte_length, "byteLength", triglav::u32)
TG_META_OPTIONAL_PROPERTY(uri, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(NS(triglav, gltf), Document)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(asset, triglav::gltf::Asset)
TG_META_PROPERTY(scene, triglav::u32)
TG_META_ARRAY_PROPERTY(scenes, triglav::gltf::Scene)
TG_META_ARRAY_PROPERTY(nodes, triglav::gltf::Node)
TG_META_ARRAY_PROPERTY(meshes, triglav::gltf::Mesh)
TG_META_ARRAY_PROPERTY(accessors, triglav::gltf::Accessor)
TG_META_ARRAY_PROPERTY(materials, triglav::gltf::Material)
TG_META_ARRAY_PROPERTY(textures, triglav::gltf::Texture)
TG_META_ARRAY_PROPERTY(images, triglav::gltf::Image)
TG_META_ARRAY_PROPERTY(samplers, triglav::gltf::Sampler)
TG_META_ARRAY_PROPERTY_STR(buffer_views, "bufferViews", triglav::gltf::BufferView)
TG_META_ARRAY_PROPERTY(buffers, triglav::gltf::Buffer)
TG_META_CLASS_END
#undef TG_TYPE
