#pragma once

#if defined(__GNUC__) || defined(__clang__)
#  define LIBTOKAMAP_UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#  define LIBTOKAMAP_UNREACHABLE __assume(0);
#else
#  define LIBTOKAMAP_UNREACHABLE
#endif
