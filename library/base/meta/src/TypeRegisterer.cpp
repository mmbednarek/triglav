#include "Meta.hpp"

#include "TypeRegistry.hpp"

namespace triglav::meta {

TypeRegisterer::TypeRegisterer(Type type)
{
   TypeRegistry::the().register_type(std::move(type));
}

}// namespace triglav::meta