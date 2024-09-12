#include "GraphicsApi.hpp"

#include <fmt/core.h>

namespace triglav::graphics_api {

Exception::Exception(const Status status, const std::string_view invoked_function) :
   status(status),
   invoked_function(std::string(invoked_function))
{
}

const char* Exception::what() const noexcept
{
   return "graphics_api exception";
}

}