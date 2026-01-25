#include "Meta.hpp"

namespace triglav::meta {

using namespace name_literals;

Box::Box(void* handle, const Name name, const std::span<Member> members) :
    ClassRef(handle, name, members)
{
}

Box::~Box()
{
   this->checked_call<void>("destroy"_name, this->raw_handle());
}

Box::Box(const Box& other) :
    ClassRef(other.checked_call<void*>("copy"_name, other.raw_handle()), other.type(), other.m_members)
{
}

Box& Box::operator=(const Box& other)
{
   dynamic_cast<Ref&>(*this) = Ref{other.checked_call<void*>("copy"_name, other.raw_handle()), other.type()};
   m_members = other.m_members;
   return *this;
}

}// namespace triglav::meta
