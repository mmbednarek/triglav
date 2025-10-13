#pragma once

#include <cassert>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace triglav::graphics_api {
struct NoParent
{};

template<typename TWrapped, auto TConstructor, auto TDestructor, typename TParentObject = NoParent>
class WrappedObject
{
 public:
   constexpr WrappedObject()
      requires(std::is_same_v<TParentObject, NoParent>)
   = default;

   explicit constexpr WrappedObject(TParentObject parent) :
       m_parent(parent)
   {
   }

   WrappedObject(const WrappedObject& other) = delete;
   WrappedObject& operator=(const WrappedObject& other) = delete;

   WrappedObject(WrappedObject&& other) noexcept :
       m_wrapped(std::exchange(other.m_wrapped, nullptr)),
       m_parent(std::exchange(other.m_parent, TParentObject{}))
   {
   }

   WrappedObject& operator=(WrappedObject&& other) noexcept
   {
      if (this == &other)
         return *this;

      this->destroy(m_wrapped);

      m_wrapped = std::exchange(other.m_wrapped, nullptr);
      m_parent = std::exchange(other.m_parent, TParentObject{});
      return *this;
   }

   constexpr ~WrappedObject()
   {
      this->destroy(m_wrapped);
   }

   [[nodiscard]] constexpr TWrapped& operator*()
   {
      return m_wrapped;
   }

   [[nodiscard]] constexpr const TWrapped& operator*() const
   {
      return m_wrapped;
   }

   [[nodiscard]] constexpr TParentObject& parent()
   {
      return m_parent;
   }

   [[nodiscard]] constexpr const TParentObject& parent() const
   {
      return m_parent;
   }

   template<typename... TArgs>
   auto construct(TArgs&&... args)
   {
      const auto old_ptr = m_wrapped;
      VkResult result{};

      if constexpr (std::is_same_v<TParentObject, NoParent>) {
         result = TConstructor(std::forward<TArgs>(args)..., nullptr, &m_wrapped);
      } else {
         result = TConstructor(m_parent, std::forward<TArgs>(args)..., nullptr, &m_wrapped);
      }

      this->destroy(old_ptr);

      return result;
   }

   void take_ownership(TWrapped wrapped)
   {
      this->destroy(m_wrapped);
      m_wrapped = wrapped;
   }

 private:
   void destroy(TWrapped wrapped)
   {
      if (wrapped == nullptr)
         return;

      if constexpr (std::is_same_v<TParentObject, NoParent>) {
         TDestructor(wrapped, nullptr);
      } else {
         TDestructor(m_parent, wrapped, nullptr);
      }
   }

   TWrapped m_wrapped{};
   TParentObject m_parent{};
};

template<typename TResult, auto TGetter, typename... TArgs>
std::vector<TResult> get_vulkan_items(TArgs&&... args)
{
   uint32_t count = 0;
   TGetter(std::forward<TArgs>(args)..., &count, nullptr);

   std::vector<TResult> result(count);
   TGetter(std::forward<TArgs>(args)..., &count, result.data());

   return result;
}

}// namespace triglav::graphics_api

#define DECLARE_VLK_WRAPPED_OBJECT(object)                                                                 \
   namespace vulkan {                                                                                      \
   using object = ::triglav::graphics_api::WrappedObject<Vk##object, vkCreate##object, vkDestroy##object>; \
   }
#define DECLARE_VLK_WRAPPED_CHILD_OBJECT(object, parent)                                                               \
   namespace vulkan {                                                                                                  \
   using object = ::triglav::graphics_api::WrappedObject<Vk##object, vkCreate##object, vkDestroy##object, Vk##parent>; \
   }
#define DECLARE_VLK_ENUMERATOR(name, object, func)                                                  \
   namespace vulkan {                                                                               \
   template<typename... TArgs>                                                                      \
   auto name(TArgs&&... args)                                                                       \
   {                                                                                                \
      return ::triglav::graphics_api::get_vulkan_items<object, func>(std::forward<TArgs>(args)...); \
   }                                                                                                \
   }
