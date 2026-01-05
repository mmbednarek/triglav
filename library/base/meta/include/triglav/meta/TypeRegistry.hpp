#pragma once

#include "Meta.hpp"

#include <map>
#include <string>

namespace triglav::meta {

class TypeRegistry
{
 public:
   void register_type(Type tp);
   [[nodiscard]] Box create_box(Name type) const;

   static TypeRegistry& the();
   [[nodiscard]] const Type& type_info(Name type_name) const;

 private:
   std::map<Name, Type> m_types;
};

}// namespace triglav::meta