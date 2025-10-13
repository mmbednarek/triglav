#include "DynLibrary.hpp"

#include <windows.h>

namespace triglav::io {

LibraryHandle load_library(const std::string_view path)
{
   return LoadLibraryA(path.data());
}

void close_library(const LibraryHandle handle)
{
   FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

SymbolHandle find_library_symbol(const LibraryHandle handle, const std::string_view symbol)
{
   return reinterpret_cast<SymbolHandle>(GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol.data()));
}

}// namespace triglav::io
