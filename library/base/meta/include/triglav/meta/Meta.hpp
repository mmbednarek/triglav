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

   ClassMemberType type;
   Name name;
   std::string identifier;
   union
   {
      FunctionPayload function;
      PropertyPayload property;
   };
};

enum class TypeVariant
{
   Primitive,
   Class,
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

class Ref
{
 public:
   Ref(void* handle, std::span<ClassMember> members);

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
   TProp& property(const Name name) const
   {
      const auto* member = this->find_member(name);
      assert(member != nullptr && member->type == ClassMemberType::Property);
      return *reinterpret_cast<TProp*>(static_cast<char*>(m_handle) + member->property.offset);
   }

   template<typename TVisitor>
   void visit_properties(TVisitor visitor)
   {
      for (const auto& mem : m_members) {
         if (mem.type == ClassMemberType::Property) {
            visitor(mem.identifier, mem.property.type_name, mem.property.offset);
         }
      }
   }

 protected:
   void* m_handle{};
   std::span<ClassMember> m_members;
};

class Box : public Ref
{
 public:
   Box(void* handle, std::span<ClassMember> members);
   ~Box();

   Box(const Box& other);
   Box& operator=(const Box& other);
};

}// namespace triglav::meta


#define TG_META_CLASS_BEGIN                                                                                                             \
   std::array TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER)                                                                          \
   {                                                                                                                                    \
      ::triglav::meta::ClassMember{                                                                                                     \
         .type = ::triglav::meta::ClassMemberType::Function,                                                                            \
         .name = ::triglav::make_name_id("destroy"),                                                                                    \
         .identifier = "destroy",                                                                                                       \
         .function =                                                                                                                    \
            {                                                                                                                           \
               .pointer =                                                                                                               \
                  reinterpret_cast<void*>(+[](void* handle) { static_cast<::TG_CLASS_NS::TG_CLASS_NAME*>(handle)->~TG_CLASS_NAME(); }), \
            },                                                                                                                          \
      },                                                                                                                                \
         ::triglav::meta::ClassMember{                                                                                                  \
            .type = ::triglav::meta::ClassMemberType::Function,                                                                         \
            .name = ::triglav::make_name_id("copy"),                                                                                    \
            .identifier = "copy",                                                                                                       \
            .function =                                                                                                                 \
               {                                                                                                                        \
                  .pointer = reinterpret_cast<void*>(+[](void* handle) -> void* {                                                       \
                     if constexpr (std::copyable<::TG_CLASS_NS::TG_CLASS_NAME>) {                                                       \
                        return new ::TG_CLASS_NS::TG_CLASS_NAME(*static_cast<const ::TG_CLASS_NS::TG_CLASS_NAME*>(handle));             \
                     } else {                                                                                                           \
                        return nullptr;                                                                                                 \
                     }                                                                                                                  \
                  }),                                                                                                                   \
               },                                                                                                                       \
         },

#define TG_META_CLASS_END                                                                      \
   }                                                                                           \
   ;                                                                                           \
   static ::triglav::meta::TypeRegisterer TG_CONCAT(TG_META_REGISTERER_, TG_CLASS_IDENTIFIER){ \
      {.name = TG_STRING(TG_CLASS_NS::TG_CLASS_NAME),                                          \
       .variant = ::triglav::meta::TypeVariant::Class,                                         \
       .members = TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER),                            \
       .factory = +[]() -> void* { return new TG_CLASS_NS::TG_CLASS_NAME(); }}};               \
   ::triglav::meta::Ref TG_CLASS_NS::TG_CLASS_NAME::to_meta_ref()                              \
   {                                                                                           \
      return {this, TG_CONCAT(TG_META_MEMBERS_, TG_CLASS_IDENTIFIER)};                         \
   }

#define TG_META_METHOD0(method_name)                                                                                                      \
   ::triglav::meta::ClassMember{                                                                                                          \
      .type = ::triglav::meta::ClassMemberType::Function,                                                                                 \
      .name = ::triglav::make_name_id(#method_name),                                                                                      \
      .identifier = #method_name,                                                                                                         \
      .function =                                                                                                                         \
         {                                                                                                                                \
            .pointer = reinterpret_cast<void*>(+[](void* handle) { static_cast<::TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(); }), \
         },                                                                                                                               \
   },

#define TG_META_METHOD0_R(method_name, ret_type)                                                                             \
   ::triglav::meta::ClassMember{                                                                                             \
      .type = ::triglav::meta::ClassMemberType::Function,                                                                    \
      .name = ::triglav::make_name_id(#method_name),                                                                         \
      .identifier = #method_name,                                                                                            \
      .function =                                                                                                            \
         {                                                                                                                   \
            .pointer = reinterpret_cast<void*>(                                                                              \
               +[](void* handle) -> ret_type { return static_cast<::TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(); }), \
         },                                                                                                                  \
   },

#define TG_META_METHOD1(method_name, arg0_ty)                                                                               \
   ::triglav::meta::ClassMember{                                                                                            \
      .type = ::triglav::meta::ClassMemberType::Function,                                                                   \
      .name = ::triglav::make_name_id(#method_name),                                                                        \
      .identifier = #method_name,                                                                                           \
      .function =                                                                                                           \
         {                                                                                                                  \
            .pointer = reinterpret_cast<void*>(                                                                             \
               +[](void* handle, arg0_ty arg0) { static_cast<::TG_CLASS_NS::TG_CLASS_NAME*>(handle)->method_name(arg0); }), \
         },                                                                                                                 \
   },

#define TG_META_PROPERTY(property_name, property_type)                       \
   ::triglav::meta::ClassMember{                                             \
      .type = ::triglav::meta::ClassMemberType::Property,                    \
      .name = ::triglav::make_name_id(#property_name),                       \
      .identifier = #property_name,                                          \
      .property =                                                            \
         {                                                                   \
            .type_name = ::triglav::make_name_id(TG_STRING(property_type)),  \
            .offset = offsetof(::TG_CLASS_NS::TG_CLASS_NAME, property_name), \
         },                                                                  \
   },

#define TG_META_BODY(class_name)       \
 public:                               \
   using Self = class_name;            \
   ::triglav::meta::Ref to_meta_ref(); \
                                       \
 private:
