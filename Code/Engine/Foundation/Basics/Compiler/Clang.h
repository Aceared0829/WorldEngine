#pragma once

#if defined(__clang__) && !defined(_MSC_VER)

#  undef W_COMPILER_CLANG
#  define W_COMPILER_CLANG W_ON

#  define W_ALWAYS_INLINE __attribute__((always_inline)) inline
#  if W_ENABLED(W_COMPILE_FOR_DEBUG)
#    define W_FORCE_INLINE inline
#  else
#    define W_FORCE_INLINE __attribute__((always_inline)) inline
#  endif

#  if __has_builtin(__builtin_debugtrap)
#    define W_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif __has_builtin(__debugbreak)
#    define W_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }
#  else
#    include <signal.h>
#    if defined(SIGTRAP)
#      define W_DEBUG_BREAK \
        {                    \
          raise(SIGTRAP);    \
        }
#    else
#      define W_DEBUG_BREAK \
        {                    \
          raise(SIGABRT);    \
        }
#    endif
#  endif

#  define W_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define W_SOURCE_LINE __LINE__
#  define W_SOURCE_FILE __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef W_COMPILE_FOR_DEBUG
#    define W_COMPILE_FOR_DEBUG W_ON
#  endif

#  define W_WARNING_PUSH() _Pragma("clang diagnostic push")
#  define W_WARNING_POP() _Pragma("clang diagnostic pop")
#  define W_WARNING_DISABLE_CLANG(_x) _Pragma(W_PP_STRINGIFY(clang diagnostic ignored _x))

#  define W_DECL_EXPORT [[gnu::visibility("default")]]
#  define W_DECL_IMPORT [[gnu::visibility("default")]]
#  define W_DECL_EXPORT_FRIEND
#  define W_DECL_IMPORT_FRIEND

#elif defined(__clang__)

// Clang in MSVC compatibility mode (clang-cl, or the clang driver targeting *-windows-msvc).
// Everything else comes from MSVC.h, but clang's warning suppressions still have to work,
// because the MSVC warning pragmas have no effect on the clang frontend.
#  define W_WARNING_DISABLE_CLANG(_x) _Pragma(W_PP_STRINGIFY(clang diagnostic ignored _x))

#else

#  define W_WARNING_DISABLE_CLANG(_x)

#endif
