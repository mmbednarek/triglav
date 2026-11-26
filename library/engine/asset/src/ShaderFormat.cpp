#include "ShaderFormat.hpp"

#include "triglav/io/File.hpp"
#include "triglav/meta/Binary.hpp"

#define TG_TYPE(ns) ns(triglav, ns(asset, BytecodeFormat))
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(SPIRV)
TG_META_ENUM_VALUE(DXIL)
TG_META_ENUM_VALUE(MSL)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(ns) ns(triglav, ns(asset, ShaderBlob))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(format, triglav::asset::BytecodeFormat)
TG_META_ARRAY_PROPERTY(data, triglav::u8)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(ns) ns(triglav, ns(asset, ShaderPropertyType))
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Texture2D)
TG_META_ENUM_VALUE(Sampler2D)
TG_META_ENUM_VALUE(SamplerState)
TG_META_ENUM_VALUE(ConstantBuffer)
TG_META_ENUM_VALUE(StructuredBuffer)
TG_META_ENUM_VALUE(RWStructuredBuffer)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(ns) ns(triglav, ns(asset, ShaderProperty))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, triglav::Name)
TG_META_PROPERTY(type, triglav::asset::ShaderPropertyType)
TG_META_PROPERTY(is_array, bool)
TG_META_PROPERTY(vulkan_descriptor_set, triglav::u32)
TG_META_PROPERTY(vulkan_descriptor_binding, triglav::u32)
TG_META_PROPERTY(d3d12_heap_offset, triglav::u32)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(ns) ns(triglav, ns(asset, ShaderData))
TG_META_CLASS_BEGIN
TG_META_ARRAY_PROPERTY(properties, triglav::asset::ShaderProperty)
TG_META_ARRAY_PROPERTY(blobs, triglav::asset::ShaderBlob)
TG_META_CLASS_END
#undef TG_TYPE
