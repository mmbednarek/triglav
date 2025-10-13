#pragma once

#include <string_view>

namespace triglav::io {

using LibraryHandle = void*;
using SymbolHandle = void*;

LibraryHandle load_library(std::string_view path);
void close_library(LibraryHandle handle);
SymbolHandle find_library_symbol(LibraryHandle handle, std::string_view symbol);

class DynLibrary
{
 public:
   explicit DynLibrary(std::string_view path);
   ~DynLibrary();

   DynLibrary(const DynLibrary& other) = delete;
   DynLibrary(DynLibrary&& other) noexcept = delete;
   DynLibrary& operator=(const DynLibrary& other) = delete;
   DynLibrary& operator=(DynLibrary&& other) noexcept = delete;

   template<typename TResult, typename... TArgs>
   TResult call(const std::string_view symbol, TArgs&&... args)
   {
      auto sym = find_library_symbol(m_handle, symbol);
      return reinterpret_cast<TResult (*)(TArgs...)>(sym)(std::forward<TArgs>(args)...);
   }

 private:
   LibraryHandle m_handle;
};

}// namespace triglav::io