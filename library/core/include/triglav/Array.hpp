#pragma once

#include <array>
#include <cstdint>

namespace triglav {

template<typename T, uint32_t CCount, typename TFactory>
std::array<T, CCount> initialize_array(TFactory &factory)
{
   std::array<char, CCount * sizeof(T)> values{};

   for (uint32_t i = 0; i < CCount; ++i) {
      reinterpret_cast<T *>(values.data())[i] = factory();
   }

   return std::move(*reinterpret_cast<std::array<T, CCount> *>(&values));
}

}// namespace triglav