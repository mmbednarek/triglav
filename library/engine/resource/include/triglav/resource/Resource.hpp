#pragma once

#include "triglav/Name.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace triglav::resource {

struct ResourceProperties
{
   std::map<Name, std::string> properties;

   void add(Name key, const std::string_view value)
   {
      properties.emplace(key, std::string{value});
   }

   [[nodiscard]] std::string get_string(Name key) const
   {
      auto it = properties.find(key);
      if (it == properties.end()) {
         return {};
      }
      return it->second;
   }

   [[nodiscard]] bool get_bool(Name key, bool defValue = false) const
   {
      const auto value = this->get_string(key);
      if (value == "off") {
         return false;
      } else if (value == "on") {
         return true;
      }
      return defValue;
   }

   [[nodiscard]] std::optional<float> get_float_opt(Name key) const
   {
      const auto value = this->get_string(key);
      if (value.empty()) {
         return std::nullopt;
      }

      return std::stof(value);
   }
};

}// namespace triglav::resource