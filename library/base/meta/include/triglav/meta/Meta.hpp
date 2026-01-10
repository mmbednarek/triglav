#pragma once

#include "triglav/EnumFlags.hpp"
#include "triglav/Int.hpp"
#include "triglav/Macros.hpp"
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
   Array = (1 << 5),// Implies property
};

TRIGLAV_DECL_FLAGS(MemberRole)

struct ClassMember
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
      void (*append)(void* handle, const void* value);                 // maybe null if readonly or static
   };

   struct Property
   {
      Name type_name;
      union
      {
         PropertyOffset offset;    // available if !(role_flags & Array) && !(role_flags & Indirect)
         PropertyIndirect indirect;// available if role_flags & Indirect
         PropertyArray array;      // available if role_flags & Array
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
};

struct Type
{
   std::string name;
   TypeVariant variant;
   std::span<ClassMember> members;
   void* (*factory)();
};

struct TypeRegisterer
{
   explicit TypeRegisterer(Type type);
};

template<typename T>
struct PropertyAccessor
{
   PropertyAccessor& operator=(T&& value)
   {
      if (this->set != nullptr) {
         this->set(this->handle, std::forward<T>(value));
      } else if (this->setref != nullptr) {
         this->setref(this->handle, &value);
      } else {
         *static_cast<T*>(this->handle) = std::forward<T>(value);
      }
      return *this;
   }

   PropertyAccessor& operator=(const T& value)
   {
      if (this->set != nullptr) {
         this->set(this->handle, value);
      } else if (this->setref != nullptr) {
         this->setref(this->handle, &value);
      } else {
         *static_cast<T*>(this->handle) = value;
      }
      return *this;
   }

   operator T() const
   {
      if (this->get != nullptr) {
         return this->get(this->handle);
      } else if (this->getref != nullptr) {
         return *static_cast<const T*>(this->getref(this->handle));
      } else {
         return *static_cast<T*>(this->handle);
      }
   }

   const T& operator*() const
   {
      if (this->getref != nullptr) {
         return *static_cast<const T*>(this->getref(this->handle));
      } else {
         return *static_cast<T*>(this->handle);
      }
   }

   const T* operator->() const
   {
      if (this->getref != nullptr) {
         return static_cast<const T*>(this->getref(this->handle));
      } else {
         return static_cast<T*>(this->handle);
      }
   }

   void* handle;
   T (*get)(void* handle);
   void (*set)(void* handle, T value);
   const void* (*getref)(void* handle);
   void (*setref)(void* handle, const void* value);
};

class ArrayRef
{
 public:
   ArrayRef(void* handle, Name contained_type, const ClassMember::PropertyArray& prop_array);

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
   void append(const T& value)
   {
      m_prop_array.append(m_handle, static_cast<const void*>(&value));
   }

 private:
   void* m_handle;
   Name m_contained_type;
   const ClassMember::PropertyArray& m_prop_array;
};

class Ref
{
 public:
   Ref(void* handle, Name name, std::span<ClassMember> members);

   [[nodiscard]] const ClassMember* find_member(Name name) const;

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
      const auto* member = this->find_member(name);
      assert(member != nullptr && member->role_flags & MemberRole::Property);
      if (member->role_flags & MemberRole::Indirect) {
         if (member->role_flags & MemberRole::Reference) {
            return PropertyAccessor<TProp>{m_handle, nullptr, nullptr,
                                           reinterpret_cast<const void* (*)(void*)>(member->property.indirect.get),
                                           reinterpret_cast<void (*)(void*, const void*)>(member->property.indirect.set)};
         } else {
            return PropertyAccessor<TProp>{m_handle, reinterpret_cast<TProp (*)(void*)>(member->property.indirect.get),
                                           reinterpret_cast<void (*)(void*, TProp)>(member->property.indirect.set)};
         }
      }

      return PropertyAccessor<TProp>{static_cast<char*>(m_handle) + member->property.offset.offset, nullptr, nullptr};
   }

   template<typename TVisitor>
   void visit_properties(TVisitor visitor) const
   {
      for (const auto& mem : m_members) {
         if (mem.role_flags & MemberRole::Property) {
            visitor(mem.identifier, mem.property.type_name);
         }
      }
   }

   [[nodiscard]] bool is_array_property(Name name) const;

   [[nodiscard]] Ref property_ref(Name name) const;
   [[nodiscard]] ArrayRef property_array_ref(Name name) const;

   [[nodiscard]] Name type() const;

   template<typename T>
   [[nodiscard]] T& as() const
   {
      return *static_cast<T*>(m_handle);
   }

 protected:
   void* m_handle{};
   Name m_type;
   std::span<ClassMember> m_members;
};

class Box : public Ref
{
 public:
   Box(void* handle, Name name, std::span<ClassMember> members);
   ~Box();

   Box(const Box& other);
   Box& operator=(const Box& other);
};

}// namespace triglav::meta


#define TG_META_CLASS_BEGIN                                                                                                           \
   std::array TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER)                                                                        \
   {                                                                                                                                  \
      ::triglav::meta::ClassMember{                                                                                                   \
         .role_flags = ::triglav::meta::MemberRole::Function,                                                                         \
         .name = ::triglav::make_name_id("destroy"),                                                                                  \
         .identifier = "destroy",                                                                                                     \
         .function =                                                                                                                  \
            {                                                                                                                         \
               .pointer =                                                                                                             \
                  reinterpret_cast<void*>(+[](void* handle) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->~TG_CLASS_NAME(); }), \
            },                                                                                                                        \
      },                                                                                                                              \
         ::triglav::meta::ClassMember{                                                                                                \
            .role_flags = ::triglav::meta::MemberRole::Function,                                                                      \
            .name = ::triglav::make_name_id("copy"),                                                                                  \
            .identifier = "copy",                                                                                                     \
            .function =                                                                                                               \
               {                                                                                                                      \
                  .pointer = reinterpret_cast<void*>(+[](void* handle) -> void* {                                                     \
                     if constexpr (std::copyable<TG_CLASS_NS::TG_CLASS_NAME>) {                                                       \
                        return new TG_CLASS_NS::TG_CLASS_NAME(*static_cast<const TG_CLASS_NS::TG_CLASS_NAME*>(handle));               \
                     } else {                                                                                                         \
                        return nullptr;                                                                                               \
                     }                                                                                                                \
                  }),                                                                                                                 \
               },                                                                                                                     \
         },

#define TG_META_CLASS_END                                                                                                              \
   }                                                                                                                                   \
   ;                                                                                                                                   \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_CLASS_IDENTIFIER){                                         \
      {.name = TG_STRING(TG_CLASS_NS::TG_CLASS_NAME),                                                                                  \
       .variant = ::triglav::meta::TypeVariant::Class,                                                                                 \
       .members = TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER),                                                                    \
       .factory = +[]() -> void* { return new TG_CLASS_NS::TG_CLASS_NAME(); }}};                                                       \
   auto TG_CLASS_NS::TG_CLASS_NAME::to_meta_ref() -> ::triglav::meta::Ref                                                              \
   {                                                                                                                                   \
      return {this, ::triglav::make_name_id(TG_STRING(TG_CLASS_NS::TG_CLASS_NAME)), TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER)}; \
   }

#define TG_META_METHOD0(method_name)                                                                                                    \
   ::triglav::meta::ClassMember{                                                                                                        \
      .role_flags = ::triglav::meta::MemberRole::Function,                                                                              \
      .name = ::triglav::make_name_id(#method_name),                                                                                    \
      .identifier = #method_name,                                                                                                       \
      .function =                                                                                                                       \
         {                                                                                                                              \
            .pointer = reinterpret_cast<void*>(+[](void* handle) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(); }), \
         },                                                                                                                             \
   },

#define TG_META_METHOD0_R(method_name, ret_type)                                                                           \
   ::triglav::meta::ClassMember{                                                                                           \
      .role_flags = ::triglav::meta::MemberRole::Function,                                                                 \
      .name = ::triglav::make_name_id(#method_name),                                                                       \
      .identifier = #method_name,                                                                                          \
      .function =                                                                                                          \
         {                                                                                                                 \
            .pointer = reinterpret_cast<void*>(                                                                            \
               +[](void* handle) -> ret_type { return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(); }), \
         },                                                                                                                \
   },

#define TG_META_METHOD1(method_name, arg0_ty)                                                                             \
   ::triglav::meta::ClassMember{                                                                                          \
      .role_flags = ::triglav::meta::MemberRole::Function,                                                                \
      .name = ::triglav::make_name_id(#method_name),                                                                      \
      .identifier = #method_name,                                                                                         \
      .function =                                                                                                         \
         {                                                                                                                \
            .pointer = reinterpret_cast<void*>(                                                                           \
               +[](void* handle, arg0_ty arg0) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(arg0); }), \
         },                                                                                                               \
   },

#define TG_META_PROPERTY(property_name, property_type)                           \
   ::triglav::meta::ClassMember{                                                 \
      .role_flags = ::triglav::meta::MemberRole::Property,                       \
      .name = ::triglav::make_name_id(#property_name),                           \
      .identifier = #property_name,                                              \
      .property =                                                                \
         {                                                                       \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),      \
            .offset =                                                            \
               {                                                                 \
                  .offset = offsetof(TG_CLASS_NS::TG_CLASS_NAME, property_name), \
               },                                                                \
         },                                                                      \
   },

#define TG_PROPERTY_TYPE(class, property) std::decay_t<decltype(static_cast<class*>(nullptr)->property)>

#define TG_META_ARRAY_PROPERTY(property_name, property_type)                                                                             \
   ::triglav::meta::ClassMember{                                                                                                         \
      .role_flags = ::triglav::meta::MemberRole::Property | ::triglav::meta::MemberRole::Array,                                          \
      .name = ::triglav::make_name_id(#property_name),                                                                                   \
      .identifier = #property_name,                                                                                                      \
      .property =                                                                                                                        \
         {                                                                                                                               \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                                              \
            .array = {                                                                                                                   \
               .count = [](void* handle) -> size_t {                                                                                     \
                  return static_cast<const TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name.size();                                   \
               },                                                                                                                        \
               .get_n = [](void* handle, size_t index) -> void* {                                                                        \
                  return &static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name.at(index);                                     \
               },                                                                                                                        \
               .set_n =                                                                                                                  \
                  [](void* handle, size_t index, const void* value) {                                                                    \
                     static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name.at(index) =                                         \
                        *static_cast<const property_type*>(value);                                                                       \
                  },                                                                                                                     \
               .append =                                                                                                                 \
                  [](void* handle, const void* value) {                                                                                  \
                     if constexpr (::triglav::HasPushBack<TG_PROPERTY_TYPE(TG_CLASS_NS::TG_CLASS_NAME, property_name), property_type>) { \
                        static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name.push_back(                                       \
                           *static_cast<const property_type*>(value));                                                                   \
                     }                                                                                                                   \
                  },                                                                                                                     \
            },                                                                                                                           \
         },                                                                                                                              \
   },

#define TG_META_INDIRECT(property_name, property_type)                                                                      \
   ::triglav::meta::ClassMember{                                                                                            \
      .role_flags = ::triglav::meta::MemberRole::Indirect | ::triglav::meta::MemberRole::Property,                          \
      .name = ::triglav::make_name_id(#property_name),                                                                      \
      .identifier = #property_name,                                                                                         \
      .property = {.type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                          \
                   .indirect =                                                                                              \
                      {                                                                                                     \
                         .get = reinterpret_cast<void*>(+[](void* handle) -> property_type {                                \
                            return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name();                       \
                         }),                                                                                                \
                         .set = reinterpret_cast<void*>(+[](void* handle, property_type value) {                            \
                            return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->TG_CONCAT(set_, property_name)(value); \
                         }),                                                                                                \
                      }},                                                                                                   \
   },

#define TG_META_INDIRECT_REF(property_name, property_type)                                                                       \
   ::triglav::meta::ClassMember{                                                                                                 \
      .role_flags =                                                                                                              \
         ::triglav::meta::MemberRole::Indirect | ::triglav::meta::MemberRole::Reference | ::triglav::meta::MemberRole::Property, \
      .name = ::triglav::make_name_id(#property_name),                                                                           \
      .identifier = #property_name,                                                                                              \
      .property = {.type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                               \
                   .indirect =                                                                                                   \
                      {                                                                                                          \
                         .get = reinterpret_cast<void*>(+[](void* handle) -> const void* {                                       \
                            return static_cast<const void*>(&static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name()); \
                         }),                                                                                                     \
                         .set = reinterpret_cast<void*>(+[](void* handle, const void* value) {                                   \
                            return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->TG_CONCAT(set_, property_name)(             \
                               *static_cast<const property_type*>(value));                                                       \
                         }),                                                                                                     \
                      }},                                                                                                        \
   },

#define TG_META_ENUM_BEGIN                                    \
   std::array TG_CONCAT(TG_META_MEMBERS_, TG_ENUM_IDENTIFIER) \
   {

#define TG_META_ENUM_END                                                                       \
   }                                                                                           \
   ;                                                                                           \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_ENUM_IDENTIFIER){{ \
      .name = TG_STRING(TG_CLASS_NS::TG_ENUM_NAME),                                            \
      .variant = ::triglav::meta::TypeVariant::Enum,                                           \
      .members = TG_CONCAT(TG_META_MEMBERS_, TG_ENUM_IDENTIFIER),                              \
      .factory = +[]() -> void* { return new TG_CLASS_NS::TG_ENUM_NAME(); },                   \
   }};

#define TG_META_ENUM_VALUE(enum_value_name)                                                     \
   ::triglav::meta::ClassMember{                                                                \
      .role_flags = ::triglav::meta::MemberRole::EnumValue,                                     \
      .name = ::triglav::make_name_id(TG_STRING(enum_value_name)),                              \
      .identifier = TG_STRING(enum_value_name),                                                 \
      .enum_value =                                                                             \
         {                                                                                      \
            .underlying_value = std::to_underlying(TG_CLASS_NS::TG_ENUM_NAME::enum_value_name), \
         },                                                                                     \
   },

#define TG_META_BODY(class_name)       \
 public:                               \
   using Self = class_name;            \
   ::triglav::meta::Ref to_meta_ref(); \
                                       \
 private:

#define TG_META_PRIMITIVE_LIST                 \
   TG_META_PRIMITIVE(char, char)               \
   TG_META_PRIMITIVE(int, int)                 \
   TG_META_PRIMITIVE(i8, i8)                   \
   TG_META_PRIMITIVE(u8, u8)                   \
   TG_META_PRIMITIVE(i16, i16)                 \
   TG_META_PRIMITIVE(u16, u16)                 \
   TG_META_PRIMITIVE(i32, i32)                 \
   TG_META_PRIMITIVE(u32, u32)                 \
   TG_META_PRIMITIVE(i64, i64)                 \
   TG_META_PRIMITIVE(u64, u64)                 \
   TG_META_PRIMITIVE(float, float)             \
   TG_META_PRIMITIVE(double, double)           \
   TG_META_PRIMITIVE(std__string, std::string) \
   TG_META_PRIMITIVE(std__string_view, std::string_view)
