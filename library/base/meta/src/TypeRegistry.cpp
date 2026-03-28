#include "TypeRegistry.hpp"

namespace triglav::meta {

using namespace name_literals;


void TypeRegistry::register_type(Type tp)
{
   m_types.emplace(make_name_id(tp.name), std::move(tp));
}

Box TypeRegistry::create_box(const Name type) const
{
   const auto& ty = m_types.at(type);
   return {ty.factory(), type, ty.members};
}

TypeRegistry& TypeRegistry::the()
{
   void placeholder_func();
   placeholder_func();

   static TypeRegistry registry;
   return registry;
}

const Type& TypeRegistry::type_info(const Name type_name) const
{
   return m_types.at(type_name);
}

}// namespace triglav::meta
