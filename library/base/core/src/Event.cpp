#include "Event.hpp"

#include <utility>

namespace triglav {

Sink::Sink() :
    m_callback_id(NULL_CALLBACK_ID)
{
}

Sink::Sink(const u32 callback_id) :
    m_callback_id(callback_id)
{
}

Sink::~Sink()
{
   if (m_callback_id != NULL_CALLBACK_ID) {
      EventManager::the().remove_callback(m_callback_id);
   }
}

Sink::Sink(Sink&& other) noexcept :
    m_callback_id(std::exchange(other.m_callback_id, NULL_CALLBACK_ID))
{
}

Sink& Sink::operator=(Sink&& other) noexcept
{
   if (this == &other)
      return *this;

   if (m_callback_id != NULL_CALLBACK_ID) {
      EventManager::the().remove_callback(m_callback_id);
   }

   m_callback_id = std::exchange(other.m_callback_id, NULL_CALLBACK_ID);
   return *this;
}

u32 Sink::release()
{
   if (m_callback_id != NULL_CALLBACK_ID) {
      EventManager::the().remove_callback(m_callback_id);
   }

   const u32 result = m_callback_id;
   m_callback_id = NULL_CALLBACK_ID;
   return result;
}

}// namespace triglav
