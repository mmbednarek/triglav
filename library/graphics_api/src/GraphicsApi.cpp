#include "GraphicsApi.hpp"

#include <fmt/core.h>

namespace triglav::graphics_api {

Exception::Exception(Status status, const std::string_view invoked_function) :
    m_message(fmt::format("graphics-api exception: function={}, status={}", invoked_function, static_cast<int>(status)))
{
}

const char* Exception::what() const noexcept
{
   return m_message.c_str();
}

}// namespace triglav::graphics_api