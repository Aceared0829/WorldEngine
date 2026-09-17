#pragma once

#ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#  define W_CORE_INTERNAL_HEADER_ALLOWED 1
#else
#  define W_CORE_INTERNAL_HEADER_ALLOWED 0
#endif

#define W_CORE_INTERNAL_HEADER static_assert(W_CORE_INTERNAL_HEADER_ALLOWED, "This is an internal W header. Please do not #include it directly.");
