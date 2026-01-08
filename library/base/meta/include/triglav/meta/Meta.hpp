#pragma once

#include "triglav/Int.hpp"
#include "triglav/Macros.hpp"
#include "triglav/Name.hpp"

#include <concepts>
#include <span>
#include <string>

namespace triglav::meta {

enum class ClassMemberType
{
   Function,
   Property,
   IndirectProperty,
   IndirectRefProperty,
   EnumValue,
};

struct ClassMember
{
   struct FunctionPayload
   {
      void* pointer;
   };

   struct PropertyPayload
   {
      Name type_name;
      MemorySize offset;
   };

   struct IndirectPropertyPayload
   {
      Name type_name;
      void* get;
      void* set;
   };

   struct EnumValue
   {
      int underlying_value;
   };

   ClassMemberType type;
   Name name;
   std::string identifier;
   union
   {
      FunctionPayload function;
      PropertyPayload property;
      IndirectPropertyPayload indirect;
      EnumValue enum_value;
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
      } else if (this->get != nullptr) {
         return this->get(this->handle);
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

class Ref
{
 public:
   Ref(void* handle, Name name, std::span<ClassMember> members);

   [[nodiscard]] const ClassMember* find_member(Name name) const;

   template<typename TRet, typename... TArgs>
   TRet call(const Name name, TArgs&&... args) const
   {
      const auto* member = this->find_member(name);
      assert(member != nullptr && member->type == ClassMemberType::Function);
      return reinterpret_cast<TRet (*)(void*, TArgs...)>(this->find_member(name)->function.pointer)(m_handle, std::forward<TArgs>(args)...);
   }

   template<typename TRet, typename... TArgs>
   TRet checked_call(const Name name, TArgs&&... args) const
   {
      const auto* member = this->find_member(name);
      if (member == nullptr || member->type != ClassMemberType::Function) {
         return TRet{};
      }
      return reinterpret_cast<TRet (*)(void*, TArgs...)>(this->find_member(name)->function.pointer)(m_handle, std::forward<TArgs>(args)...);
   }

   template<typename TProp>
   PropertyAccessor<TProp> property(const Name name) const
   {
      const auto* member = this->find_member(name);
      assert(member != nullptr && member->type != ClassMemberType::Function);
      if (member->type == ClassMemberType::Property) {
         return PropertyAccessor<TProp>{static_cast<char*>(m_handle) + member->property.offset, nullptr, nullptr};
      }
      if (member->type == ClassMemberType::IndirectProperty) {
         return PropertyAccessor<TProp>{m_handle, reinterpret_cast<TProp (*)(void*)>(member->indirect.get),
                                        reinterpret_cast<void (*)(void*, TProp)>(member->indirect.set)};
      }
      if (member->type == ClassMemberType::IndirectRefProperty) {
         return PropertyAccessor<TProp>{m_handle, nullptr, nullptr, reinterpret_cast<const void* (*)(void*)>(member->indirect.get),
                                        reinterpret_cast<void (*)(void*, const void*)>(member->indirect.set)};
      }
      return {};
   }

   template<typename TVisitor>
   void visit_properties(TVisitor visitor) const
   {
      for (const auto& mem : m_members) {
         if (mem.type != ClassMemberType::Function) {
            visitor(mem.identifier, mem.indirect.type_name);
         }
      }
   }

   [[nodiscard]] Ref property_ref(Name name) const;

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
         .type = ::triglav::meta::ClassMemberType::Function,                                                                          \
         .name = ::triglav::make_name_id("destroy"),                                                                                  \
         .identifier = "destroy",                                                                                                     \
         .function =                                                                                                                  \
            {                                                                                                                         \
               .pointer =                                                                                                             \
                  reinterpret_cast<void*>(+[](void* handle) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->~TG_CLASS_NAME(); }), \
            },                                                                                                                        \
      },                                                                                                                              \
         ::triglav::meta::ClassMember{                                                                                                \
            .type = ::triglav::meta::ClassMemberType::Function,                                                                       \
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
      .type = ::triglav::meta::ClassMemberType::Function,                                                                               \
      .name = ::triglav::make_name_id(#method_name),                                                                                    \
      .identifier = #method_name,                                                                                                       \
      .function =                                                                                                                       \
         {                                                                                                                              \
            .pointer = reinterpret_cast<void*>(+[](void* handle) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(); }), \
         },                                                                                                                             \
   },

#define TG_META_METHOD0_R(method_name, ret_type)                                                                           \
   ::triglav::meta::ClassMember{                                                                                           \
      .type = ::triglav::meta::ClassMemberType::Function,                                                                  \
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
      .type = ::triglav::meta::ClassMemberType::Function,                                                                 \
      .name = ::triglav::make_name_id(#method_name),                                                                      \
      .identifier = #method_name,                                                                                         \
      .function =                                                                                                         \
         {                                                                                                                \
            .pointer = reinterpret_cast<void*>(                                                                           \
               +[](void* handle, arg0_ty arg0) { static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(arg0); }), \
         },                                                                                                               \
   },

#define TG_META_PROPERTY(property_name, property_type)                      \
   ::triglav::meta::ClassMember{                                            \
      .type = ::triglav::meta::ClassMemberType::Property,                   \
      .name = ::triglav::make_name_id(#property_name),                      \
      .identifier = #property_name,                                         \
      .property =                                                           \
         {                                                                  \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)), \
            .offset = offsetof(TG_CLASS_NS::TG_CLASS_NAME, property_name),  \
         },                                                                 \
   },

#define TG_META_INDIRECT(property_name, property_type)                                                                            \
   ::triglav::meta::ClassMember{                                                                                                  \
      .type = ::triglav::meta::ClassMemberType::IndirectProperty,                                                                 \
      .name = ::triglav::make_name_id(#property_name),                                                                            \
      .identifier = #property_name,                                                                                               \
      .indirect =                                                                                                                 \
         {                                                                                                                        \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                                       \
            .get = reinterpret_cast<void*>(                                                                                       \
               +[](void* handle) -> property_type { return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name(); }), \
            .set = reinterpret_cast<void*>(+[](void* handle, property_type value) {                                               \
               return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->TG_CONCAT(set_, property_name)(value);                    \
            }),                                                                                                                   \
         },                                                                                                                       \
   },

#define TG_META_INDIRECT_REF(property_name, property_type)                                                          \
   ::triglav::meta::ClassMember{                                                                                    \
      .type = ::triglav::meta::ClassMemberType::IndirectRefProperty,                                                \
      .name = ::triglav::make_name_id(#property_name),                                                              \
      .identifier = #property_name,                                                                                 \
      .indirect =                                                                                                   \
         {                                                                                                          \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),                                         \
            .get = reinterpret_cast<void*>(+[](void* handle) -> const void* {                                       \
               return static_cast<const void*>(&static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->property_name()); \
            }),                                                                                                     \
            .set = reinterpret_cast<void*>(+[](void* handle, const void* value) {                                   \
               return static_cast<TG_CLASS_NS::TG_CLASS_NAME*>(handle)->TG_CONCAT(set_, property_name)(             \
                  *static_cast<const property_type*>(value));                                                       \
            }),                                                                                                     \
         },                                                                                                         \
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
      .type = ::triglav::meta::ClassMemberType::EnumValue,                                      \
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
