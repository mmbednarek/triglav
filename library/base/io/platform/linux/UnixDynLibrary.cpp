#include "DynLibrary.hpp"

#include <dlfcn.h>

namespace triglav::io {

LibraryHandle load_library(const std::string_view path)
{
   return dlopen(path.data(), RTLD_NOW);
}

void close_library(const LibraryHandle handle)
{
   dlclose(handle);
}

SymbolHandle find_library_symbol(const LibraryHandle handle, const std::string_view symbol)
{
   return dlsym(handle, symbol.data());
}

}// namespace triglav::io