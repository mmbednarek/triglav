#pragma once

#include "Macros.hpp"

#ifdef __linux__
#define TG_OS linux
#elifdef _WIN32
#define TG_OS windows
#endif

#ifdef __clang__
#define TG_COMPILER clang
#elifdef __GNUC__
#define TG_COMPILER gcc
#elifdef _MSC_VER
#define TG_COMPILER msvc
#endif

#ifdef NDEBUG
#define TG_ADD_BUILD_SUFFIX(x) x - r
#else
#define TG_ADD_BUILD_SUFFIX(x) x
#endif

#define TG_BUILD_PROFILE TG_ADD_BUILD_SUFFIX(TG_OS - TG_COMPILER)