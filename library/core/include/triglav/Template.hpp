#pragma once

namespace triglav {

template<typename TClass>
struct ClassContainer
{
   using Type = TClass;
};

template<typename TMemberType, typename TClass>
constexpr ClassContainer<TClass> get_class_of_member(TMemberType TClass::*member)
{
   return ClassContainer<TClass>{};
}

template<typename TMemberType, typename TClass>
constexpr ClassContainer<TMemberType> get_type_of_member(TMemberType TClass::*member)
{
   return ClassContainer<TMemberType>{};
}

template<auto CMember>
using ClassOfMember = typename decltype(get_class_of_member(CMember))::Type;
template<auto CMember>
using TypeOfMember = typename decltype(get_type_of_member(CMember))::Type;

}// namespace triglav