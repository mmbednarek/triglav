#include "DynLibrary.hpp"

#include <cassert>

namespace triglav::io {

DynLibrary::DynLibrary(const std::string_view path) :
    m_handle(load_library(path))
{
   assert(m_handle != nullptr);
}

DynLibrary::~DynLibrary()
{
   close_library(m_handle);
}

}// namespace triglav::io