#pragma once

#include "triglav/EnumFlags.hpp"
#include "triglav/Int.hpp"
#include "triglav/Macros.hpp"
#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/Template.hpp"

#include <concepts>
#include <span>
#include <string>

namespace triglav::meta {

// Function / Property / EnumValue are exclusive
// Indirect / Array are exclusive
enum class MemberRole
{
   Function = (1 << 0),
   Property = (1 << 1),
   Indirect = (1 << 2),// Implies property
   Reference = (1 << 3),
   EnumValue = (1 << 4),
   Array = (1 << 5),   // Implies property
   Map = (1 << 6),     // Implies property
   Optional = (1 << 7),// Implies property
};

TRIGLAV_DECL_FLAGS(MemberRole)

struct Member
{
   struct FunctionPayload
   {
      void* pointer;
   };

   struct PropertyOffset
   {
      MemorySize offset;
   };

   struct PropertyIndirect
   {
      void* get;
      void* set;
   };

   struct PropertyArray
   {
      MemorySize (*count)(void* handle);
      void* (*get_n)(void* handle, MemorySize index);
      void (*set_n)(void* handle, MemorySize index, const void* value);// maybe null if readonly
      void* (*append)(void* handle);                                   // maybe null if readonly or static
   };

   struct PropertyMap
   {
      Name key_type;
      MemorySize (*count)(void* handle);
      const void* (*first_key)(void* handle);
      const void* (*next_key)(void* handle, const void* key);
      void* (*get_n)(void* handle, const void* key);
      void (*set_n)(void* handle, const void* key, const void* value);// maybe null if readonly
   };

   struct PropertyOptional
   {
      bool (*has_value)(void* handle);
      void* (*get)(void* handle);
      void (*set)(void* handle, void* value);
   };

   struct Property
   {
      Name type_name;
      union
      {
         PropertyOffset offset;    // available if !(role_flags & Array) && !(role_flags & Indirect)
         PropertyIndirect indirect;// available if role_flags & Indirect
         PropertyArray array;      // available if role_flags & Array
         PropertyMap map;          // available if role_flags & Map
         PropertyOptional optional;// available if role_flags & Optional
      };
   };

   struct EnumValue
   {
      int underlying_value;
   };

   MemberRoleFlags role_flags;
   Name name;
   std::string identifier;
   union
   {
      FunctionPayload function;// available if role is role_flags & Function
      Property property;       // available if role is role_flags & Property
      EnumValue enum_value;    // available if role is role_flags & EnumValue
   };
};

enum class TypeVariant
{
   Primitive,
   Class,
   Enum,
   Array,// not coming from type but from property
   Map,
   Optional,
};

struct Type
{
   std::string name;
   TypeVariant variant;
   std::span<Member> members;
   void* (*factory)();
};

struct TypeRegisterer
{
   explicit TypeRegisterer(Type type);
};

class Ref;
class ArrayRef;
class MapRef;
class OptionalRef;

class PropertyRef
{
 public:
   PropertyRef(void* handle, const Member* member);

   template<typename T>
   const T& get_ref() const
   {
      if (!(m_member->role_flags & MemberRole::Indirect)) {
         return *reinterpret_cast<const T*>(static_cast<char*>(m_handle) + m_member->property.offset.offset);
      }

      if (m_member->role_flags & MemberRole::Reference) {
         return *static_cast<const T*>(reinterpret_cast<const void* (*)(void*)>(m_member->property.indirect.get)(m_handle));
      }

      // shouldn't use getref on Indirect Non-reference property!
      assert(false);
      std::unreachable();
   }

   template<typename T>
   T get() const
   {
      if (!(m_member->role_flags & MemberRole::Indirect)) {
         return *reinterpret_cast<const T*>(static_cast<char*>(m_handle) + m_member->property.offset.offset);
      }

      if (m_member->role_flags & MemberRole::Reference) {
         return *static_cast<const T*>(reinterpret_cast<const void* (*)(void*)>(m_member->property.indirect.get)(m_handle));
      }

      return reinterpret_cast<T (*)(void*)>(m_member->property.indirect.get)(m_handle);
   }

   template<typename T>
   void set(T&& value) const
   {
      if (!(m_member->role_flags & MemberRole::Indirect)) {
         *reinterpret_cast<T*>(static_cast<char*>(m_handle) + m_member->property.offset.offset) = std::forward<T>(value);
         return;
      }

      if (m_member->role_flags & MemberRole::Reference) {
         reinterpret_cast<void (*)(void*, const void*)>(m_member->property.indirect.set)(m_handle, &value);
         return;
      }

      reinterpret_cast<void (*)(void*, T)>(m_member->property.indirect.set)(m_handle, std::forward<T>(value));
   }

   template<typename T>
   void set(const T& value) const
   {
      if (!(m_member->role_flags & MemberRole::Indirect)) {
         *reinterpret_cast<T*>(static_cast<char*>(m_handle) + m_member->property.offset.offset) = value;
         return;
      }

      if (m_member->role_flags & MemberRole::Reference) {
         reinterpret_cast<void (*)(void*, const void*)>(m_member->property.indirect.set)(m_handle, &value);
         return;
      }

      reinterpret_cast<void (*)(void*, T)>(m_member->property.indirect.set)(m_handle, value);
   }

   [[nodiscard]] std::string_view identifier() const;
   [[nodiscard]] Name name() const;
   [[nodiscard]] Ref to_ref() const;
   [[nodiscard]] Name type() const;
   [[nodiscard]] bool is_array() const;
   [[nodiscard]] bool is_map() const;
   [[nodiscard]] bool is_optional() const;
   [[nodiscard]] ArrayRef to_array_ref() const;
   [[nodiscard]] MapRef to_map_ref() const;
   [[nodiscard]] OptionalRef to_optional_ref() const;
   [[nodiscard]] TypeVariant type_variant() const;

 private:
   void* m_handle;
   const Member* m_member;
};

template<typename T>
struct PropertyAccessor
{
   PropertyAccessor& operator=(T&& value)
   {
      m_reference.set(std::forward<T>(value));
      return *this;
   }

   PropertyAccessor& operator=(const T& value)
   {
      m_reference.set(std::forward<T>(value));
      return *this;
   }

   operator T() const
   {
      return m_reference.get<T>();
   }

   const T& operator*() const
   {
      return m_reference.get_ref<T>();
   }

   const T* operator->() const
   {
      return &m_reference.get_ref<T>();
   }

   PropertyRef m_reference;
};

class ArrayRef
{
 public:
   ArrayRef(void* handle, Name contained_type, const Member::PropertyArray& prop_array);

   template<typename T>
   T& at(const MemorySize index)
   {
      return *static_cast<T*>(m_prop_array.get_n(m_handle, index));
   }

   template<typename T>
   const T& at(const MemorySize index) const
   {
      return *static_cast<const T*>(m_prop_array.get_n(m_handle, index));
   }

   template<typename T>
   void set(const MemorySize index, const T& value)
   {
      m_prop_array.set_n(m_handle, index, static_cast<const void*>(&value));
   }

   [[nodiscard]] MemorySize size() const
   {
      return m_prop_array.count(m_handle);
   }

   template<typename T>
   [[nodiscard]] T& append() const
   {
      return *static_cast<T*>(m_prop_array.append(m_handle));
   }

   [[nodiscard]] Ref at_ref(MemorySize index) const;
   [[nodiscard]] Ref append_ref() const;
   [[nodiscard]] Name type() const;
   [[nodiscard]] TypeVariant type_variant() const;

 private:
   void* m_handle;
   Name m_contained_type;
   const Member::PropertyArray& m_prop_array;
};

class MapRef
{
 public:
   MapRef(void* handle, Name value_type, const Member::PropertyMap& prop_array);

   template<typename TKey, typename TValue>
   TValue& at(const TKey& key)
   {
      return *static_cast<TValue*>(m_prop_map.get_n(m_handle, &key));
   }

   template<typename TKey, typename TValue>
   void set(const TKey& key, const TValue& value)
   {
      return m_prop_map.set_n(m_handle, &key, &value);
   }

   template<typename TKey>
   const TKey& first_key()
   {
      return *static_cast<const TKey*>(m_prop_map.first_key(m_handle));
   }

   template<typename TKey>
   const TKey& next_key(const TKey& key)
   {
      return *static_cast<const TKey*>(m_prop_map.next_key(m_handle, &key));
   }

   [[nodiscard]] Name key_type() const;
   [[nodiscard]] Name value_type() const;
   [[nodiscard]] Ref get_ref(const Ref& key) const;
   [[nodiscard]] Ref first_key_ref() const;
   [[nodiscard]] Ref next_key_ref(const Ref& key) const;

 private:
   void* m_handle;
   Name m_value_type;
   const Member::PropertyMap& m_prop_map;
};

class OptionalRef
{
 public:
   OptionalRef(void* handle, Name contained_type, const Member::PropertyOptional& prop_optional);

   [[nodiscard]] bool has_value() const;
   Ref get_ref() const;

 private:
   void* m_handle;
   Name m_contained_type;
   const Member::PropertyOptional& m_prop_optional;
};

class Ref
{
 public:
   Ref(void* handle, Name name, std::span<Member> members);
   Ref(void* handle, Name name);

   [[nodiscard]] const Member* find_member(Name name) const;

   template<typename TRet, typename... TArgs>
   TRet call(const Name name, TArgs&&... args) const
   {
      const auto* member = this->find_member(name);
      assert(member != nullptr && member->role_flags & MemberRole::Function);
      return reinterpret_cast<TRet (*)(void*, TArgs...)>(this->find_member(name)->function.pointer)(m_handle, std::forward<TArgs>(args)...);
   }

   template<typename TRet, typename... TArgs>
   TRet checked_call(const Name name, TArgs&&... args) const
   {
      const auto* member = this->find_member(name);
      if (member == nullptr || !(member->role_flags & MemberRole::Function)) {
         return TRet{};
      }
      return reinterpret_cast<TRet (*)(void*, TArgs...)>(this->find_member(name)->function.pointer)(m_handle, std::forward<TArgs>(args)...);
   }

   template<typename TProp>
   PropertyAccessor<TProp> property(const Name name) const
   {
      return PropertyAccessor<TProp>{this->property_ref(name)};
   }

   template<typename TVisitor>
   void visit_properties(TVisitor visitor) const
   {
      for (const auto& mem : m_members) {
         if (mem.role_flags & MemberRole::Property) {
            visitor(PropertyRef{m_handle, &mem});
         }
      }
   }

   [[nodiscard]] bool is_array_property(Name name) const;

   [[nodiscard]] PropertyRef property_ref(Name name) const;
   [[nodiscard]] ArrayRef property_array_ref(Name name) const;
   [[nodiscard]] MapRef property_map_ref(Name name) const;
   [[nodiscard]] bool is_nullptr() const;
   [[nodiscard]] void* raw_handle() const;

   [[nodiscard]] Name type() const;

   template<typename T>
   [[nodiscard]] T& as() const
   {
      return *static_cast<T*>(m_handle);
   }

 protected:
   void* m_handle{};
   Name m_type;
   std::span<Member> m_members;
};

class Box : public Ref
{
 public:
   Box(void* handle, Name name, std::span<Member> members);

   ~Box();

   Box(const Box& other);

   Box& operator=(const Box& other);
};
}// namespace triglav::meta

#define TG_META_JOIN_NS(x, y) x::y
#define TG_META_JOIN_IDEN(x, y) TG_CONCAT(x, TG_CONCAT(_N_, y))
#define TG_META_GET_LAST(x, y) y

#define TG_META_CLASS_BEGIN                                                                                             \
   std::array TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN))                                                   \
   {                                                                                                                    \
      ::triglav::meta::Member{                                                                                          \
         .role_flags = ::triglav::meta::MemberRole::Function,                                                           \
         .name = ::triglav::make_name_id("destroy"),                                                                    \
         .identifier = "destroy",                                                                                       \
         .function =                                                                                                    \
            {                                                                                                           \
               .pointer = reinterpret_cast<void*>(                                                                      \
                  +[](void* handle) { static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->~TG_TYPE(TG_META_GET_LAST)(); }), \
            },                                                                                                          \
      },                                                                                                                \
         ::triglav::meta::Member{                                                                                       \
            .role_flags = ::triglav::meta::MemberRole::Function,                                                        \
            .name = ::triglav::make_name_id("copy"),                                                                    \
            .identifier = "copy",                                                                                       \
            .function =                                                                                                 \
               {                                                                                                        \
                  .pointer = reinterpret_cast<void*>(+[](void* handle) -> void* {                                       \
                     if constexpr (std::copyable<TG_TYPE(TG_META_JOIN_NS)>) {                                           \
                        return new TG_TYPE(TG_META_JOIN_NS)(*static_cast<const TG_TYPE(TG_META_JOIN_NS)*>(handle));     \
                     } else {                                                                                           \
                        return nullptr;                                                                                 \
                     }                                                                                                  \
                  }),                                                                                                   \
               },                                                                                                       \
         },

#define TG_META_CLASS_END                                                                             \
   }                                                                                                  \
   ;                                                                                                  \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_TYPE(TG_META_JOIN_IDEN)){ \
      {.name = TG_STRING(TG_TYPE(TG_META_JOIN_NS)),                                                   \
       .variant = ::triglav::meta::TypeVariant::Class,                                                \
       .members = TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN)),                            \
       .factory = +[]() -> void* { return new TG_TYPE(TG_META_JOIN_NS)(); }}};                        \
   auto TG_TYPE(TG_META_JOIN_NS)::to_meta_ref() -> ::triglav::meta::Ref                               \
   {                                                                                                  \
      return {this, ::triglav::make_name_id(TG_STRING(TG_TYPE(TG_META_JOIN_NS))),                     \
              TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN))};                               \
   }

#define TG_META_FUNC_ARGS_CB(index, type_name) type_name TG_CONCAT(arg, index)
#define TG_META_FUNC_ARGS(...) TG_FOR_EACH(TG_META_FUNC_ARGS_CB, __VA_ARGS__)

#define TG_META_PASSED_ARGS_CB(index, type_name) TG_CONCAT(arg, index)
#define TG_META_PASSED_ARGS(...) TG_FOR_EACH(TG_META_PASSED_ARGS_CB, __VA_ARGS__)

#define TG_META_METHOD(method_name, ...)                                                                         \
   ::triglav::meta::Member{                                                                                      \
      .role_flags = ::triglav::meta::MemberRole::Function,                                                       \
      .name = ::triglav::make_name_id(#method_name),                                                             \
      .identifier = #method_name,                                                                                \
      .function =                                                                                                \
         {                                                                                                       \
            .pointer = reinterpret_cast<void*>(+[](void* handle __VA_OPT__(, ) TG_META_FUNC_ARGS(__VA_ARGS__)) { \
               static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->method_name(TG_META_PASSED_ARGS(__VA_ARGS__));    \
            }),                                                                                                  \
         },                                                                                                      \
   },

#define TG_META_METHOD_R(method_name, ret_type, ...)                                                                         \
   ::triglav::meta::Member{                                                                                                  \
      .role_flags = ::triglav::meta::MemberRole::Function,                                                                   \
      .name = ::triglav::make_name_id(#method_name),                                                                         \
      .identifier = #method_name,                                                                                            \
      .function =                                                                                                            \
         {                                                                                                                   \
            .pointer = reinterpret_cast<void*>(+[](void* handle __VA_OPT__(, ) TG_META_FUNC_ARGS(__VA_ARGS__)) -> ret_type { \
               return static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->method_name(TG_META_PASSED_ARGS(__VA_ARGS__));         \
            }),                                                                                                              \
         },                                                                                                                  \
   },

#define TG_META_PROPERTY(property_name, property_type)                         \
   ::triglav::meta::Member{                                                    \
      .role_flags = ::triglav::meta::MemberRole::Property,                     \
      .name = ::triglav::make_name_id(#property_name),                         \
      .identifier = #property_name,                                            \
      .property =                                                              \
         {                                                                     \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),    \
            .offset =                                                          \
               {                                                               \
                  .offset = offsetof(TG_TYPE(TG_META_JOIN_NS), property_name), \
               },                                                              \
         },                                                                    \
   },

#define TG_META_OPTIONAL_PROPERTY(property_name, property_type)                                                                        \
   ::triglav::meta::Member{                                                                                                            \
      .role_flags = ::triglav::meta::MemberRole::Property | ::triglav::meta::MemberRole::Optional,                                     \
      .name = ::triglav::make_name_id(#property_name),                                                                                 \
      .identifier = #property_name,                                                                                                    \
      .property =                                                                                                                      \
         {                                                                                                                             \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                                            \
            .optional = {                                                                                                              \
               .has_value = [](void* handle) -> bool {                                                                                 \
                  return static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.has_value();                                    \
               },                                                                                                                      \
               .get = [](void* handle) -> void* { return &static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.value(); },    \
               .set =                                                                                                                  \
                  [](void* handle, void* value) {                                                                                      \
                     static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.emplace(*static_cast<const property_type*>(value)); \
                  },                                                                                                                   \
            },                                                                                                                         \
                                                                                                                                       \
         },                                                                                                                            \
   },

#define TG_PROPERTY_TYPE(class, property) std::decay_t<decltype(static_cast<class*>(nullptr)->property)>

#define TG_META_ARRAY_PROPERTY(property_name, property_type)                                                                               \
   ::triglav::meta::Member{                                                                                                                \
      .role_flags = ::triglav::meta::MemberRole::Property | ::triglav::meta::MemberRole::Array,                                            \
      .name = ::triglav::make_name_id(#property_name),                                                                                     \
      .identifier = #property_name,                                                                                                        \
      .property =                                                                                                                          \
         {                                                                                                                                 \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                                                \
            .array = {                                                                                                                     \
               .count = [](void* handle) -> size_t { return static_cast<const TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.size(); }, \
               .get_n = [](void* handle, size_t index) -> void* {                                                                          \
                  return &static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.at(index);                                         \
               },                                                                                                                          \
               .set_n =                                                                                                                    \
                  [](void* handle, size_t index, const void* value) {                                                                      \
                     static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name.at(index) = *static_cast<const property_type*>(value);  \
                  },                                                                                                                       \
               .append = [](void* handle) -> void* {                                                                                       \
                  if constexpr (::triglav::HasPushBack<TG_PROPERTY_TYPE(TG_TYPE(TG_META_JOIN_NS), property_name), property_type>) {        \
                     auto& arr = static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name;                                            \
                     arr.push_back(property_type{});                                                                                       \
                     return &arr.at(arr.size() - 1);                                                                                       \
                  } else {                                                                                                                 \
                     return nullptr;                                                                                                       \
                  }                                                                                                                        \
               },                                                                                                                          \
            },                                                                                                                             \
         },                                                                                                                                \
   },

#define TG_META_INDIRECT(property_name, property_type)                                                                    \
   ::triglav::meta::Member{                                                                                               \
      .role_flags = ::triglav::meta::MemberRole::Indirect | ::triglav::meta::MemberRole::Property,                        \
      .name = ::triglav::make_name_id(#property_name),                                                                    \
      .identifier = #property_name,                                                                                       \
      .property = {.type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                        \
                   .indirect =                                                                                            \
                      {                                                                                                   \
                         .get = reinterpret_cast<void*>(+[](void* handle) -> property_type {                              \
                            return static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name();                       \
                         }),                                                                                              \
                         .set = reinterpret_cast<void*>(+[](void* handle, property_type value) {                          \
                            return static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->TG_CONCAT(set_, property_name)(value); \
                         }),                                                                                              \
                      }},                                                                                                 \
   },

#define TG_META_INDIRECT_REF(property_name, property_type)                                                                       \
   ::triglav::meta::Member{                                                                                                      \
      .role_flags =                                                                                                              \
         ::triglav::meta::MemberRole::Indirect | ::triglav::meta::MemberRole::Reference | ::triglav::meta::MemberRole::Property, \
      .name = ::triglav::make_name_id(#property_name),                                                                           \
      .identifier = #property_name,                                                                                              \
      .property = {.type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                               \
                   .indirect =                                                                                                   \
                      {                                                                                                          \
                         .get = reinterpret_cast<void*>(+[](void* handle) -> const void* {                                       \
                            return static_cast<const void*>(&static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->property_name());   \
                         }),                                                                                                     \
                         .set = reinterpret_cast<void*>(+[](void* handle, const void* value) {                                   \
                            return static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->TG_CONCAT(set_, property_name)(               \
                               *static_cast<const property_type*>(value));                                                       \
                         }),                                                                                                     \
                      }},                                                                                                        \
   },

#define TG_META_ENUM_BEGIN                                            \
   std::array TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN)) \
   {

#define TG_META_ENUM_END                                                                               \
   }                                                                                                   \
   ;                                                                                                   \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_TYPE(TG_META_JOIN_IDEN)){{ \
      .name = TG_STRING(TG_TYPE(TG_META_JOIN_NS)),                                                     \
      .variant = ::triglav::meta::TypeVariant::Enum,                                                   \
      .members = TG_CONCAT(TG_META_MEMBERS_, TG_TYPE(TG_META_JOIN_IDEN)),                              \
      .factory = +[]() -> void* { return new TG_TYPE(TG_META_JOIN_NS)(); },                            \
   }};

#define TG_META_ENUM_VALUE(enum_value_name)                                                    \
   ::triglav::meta::Member{                                                                    \
      .role_flags = ::triglav::meta::MemberRole::EnumValue,                                    \
      .name = ::triglav::make_name_id(TG_STRING(enum_value_name)),                             \
      .identifier = TG_STRING(enum_value_name),                                                \
      .enum_value =                                                                            \
         {                                                                                     \
            .underlying_value = std::to_underlying(TG_TYPE(TG_META_JOIN_NS)::enum_value_name), \
         },                                                                                    \
   },

#define TG_META_ENUM_VALUE_STR(enum_value_name, enum_value_str)                                \
   ::triglav::meta::Member{                                                                    \
      .role_flags = ::triglav::meta::MemberRole::EnumValue,                                    \
      .name = ::triglav::make_name_id(TG_STRING(enum_value_name)),                             \
      .identifier = (enum_value_str),                                                          \
      .enum_value =                                                                            \
         {                                                                                     \
            .underlying_value = std::to_underlying(TG_TYPE(TG_META_JOIN_NS)::enum_value_name), \
         },                                                                                    \
   },

#define TG_META_MAP_PROPERTY(prop_name, prop_key_type, value_type)                                                                     \
   ::triglav::meta::Member{                                                                                                            \
      .role_flags = ::triglav::meta::MemberRole::Property | ::triglav::meta::MemberRole::Map,                                          \
      .name = ::triglav::make_name_id(TG_STRING(prop_name)),                                                                           \
      .identifier = TG_STRING(prop_name),                                                                                              \
      .property =                                                                                                                      \
         {                                                                                                                             \
            .type_name = ::triglav::make_name_id(TG_STRING(value_type)),                                                               \
            .map = {                                                                                                                   \
               .key_type = ::triglav::make_name_id(TG_STRING(prop_key_type)),                                                          \
               .count = [](void* handle) -> size_t { return static_cast<const TG_TYPE(TG_META_JOIN_NS)*>(handle)->prop_name.size(); }, \
               .first_key = [](void* handle) -> const void* {                                                                          \
                  return &static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->prop_name.begin()->first;                                    \
               },                                                                                                                      \
               .next_key = [](void* handle, const void* key) -> const void* {                                                          \
                  auto& map = static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->prop_name;                                               \
                  const auto it = std::next(map.find(*static_cast<const prop_key_type*>(key)));                                        \
                  if (it == map.end())                                                                                                 \
                     return nullptr;                                                                                                   \
                  return &it->first;                                                                                                   \
               },                                                                                                                      \
               .get_n = [](void* handle, const void* key) -> void* {                                                                   \
                  return &static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->prop_name.at(*static_cast<const prop_key_type*>(key));       \
               },                                                                                                                      \
               .set_n =                                                                                                                \
                  [](void* handle, const void* key, const void* value) {                                                               \
                     static_cast<TG_TYPE(TG_META_JOIN_NS)*>(handle)->prop_name[*static_cast<const prop_key_type*>(key)] =              \
                        *static_cast<const value_type*>(value);                                                                        \
                  },                                                                                                                   \
            },                                                                                                                         \
         },                                                                                                                            \
   },

#define TG_META_BODY(class_name)       \
 public:                               \
   using Self = class_name;            \
   ::triglav::meta::Ref to_meta_ref(); \
                                       \
 private:

#define TG_META_STRUCT_BODY(class_name) \
   using Self = class_name;             \
   ::triglav::meta::Ref to_meta_ref();

#define TG_META_PRIMITIVE_LIST                           \
   TG_META_PRIMITIVE(char, char)                         \
   TG_META_PRIMITIVE(int, int)                           \
   TG_META_PRIMITIVE(triglav__i8, triglav::i8)           \
   TG_META_PRIMITIVE(triglav__u8, triglav::u8)           \
   TG_META_PRIMITIVE(triglav__i16, triglav::i16)         \
   TG_META_PRIMITIVE(triglav__u16, triglav::u16)         \
   TG_META_PRIMITIVE(triglav__i32, triglav::i32)         \
   TG_META_PRIMITIVE(triglav__u32, triglav::u32)         \
   TG_META_PRIMITIVE(triglav__i64, triglav::i64)         \
   TG_META_PRIMITIVE(triglav__u64, triglav::u64)         \
   TG_META_PRIMITIVE(float, float)                       \
   TG_META_PRIMITIVE(double, double)                     \
   TG_META_PRIMITIVE(std__string, std::string)           \
   TG_META_PRIMITIVE(std__string_view, std::string_view) \
   TG_META_PRIMITIVE(triglav__Vector2, triglav::Vector2) \
   TG_META_PRIMITIVE(triglav__Vector3, triglav::Vector3) \
   TG_META_PRIMITIVE(triglav__Vector4, triglav::Vector4) \
   TG_META_PRIMITIVE(triglav__Matrix4x4, triglav::Matrix4x4)
