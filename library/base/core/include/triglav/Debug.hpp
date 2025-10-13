#pragma once

#ifdef NDEBUG

#define TG_DEBUG_VAR(name, ...)

#else

#define TG_DEBUG_VAR(name, ...) \
   const auto debug_##name      \
   {                            \
      __VA_ARGS__               \
   }

#endif