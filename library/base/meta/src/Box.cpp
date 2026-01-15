#include "Meta.hpp"

namespace triglav::meta {

using namespace name_literals;

Box::Box(void* handle, const Name name, const std::span<Member> members) :
    Ref(handle, name, members)
{
}

Box::~Box()
{
   this->checked_call<void>("destroy"_name, m_handle);
}

Box::Box(const Box& other) :
    Ref(other.checked_call<void*>("copy"_name, other.m_handle), other.type(), other.m_members)
{
}

Box& Box::operator=(const Box& other)
{
   m_handle = other.checked_call<void*>("copy"_name, other.m_handle);
   m_type = other.type();
   m_members = other.m_members;
   return *this;
}

}// namespace triglav::meta
