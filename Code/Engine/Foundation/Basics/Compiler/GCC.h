#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef W_COMPILER_GCC
#  define W_COMPILER_GCC W_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define W_ALWAYS_INLINE inline
#  define W_FORCE_INLINE inline

#  if __has_builtin(__builtin_debugtrap)
#    define W_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif defined(__i386__) || defined(__x86_64__)
// Use a non-inline function to avoid C++20 extension warnings when W_ASSERT is used in constexpr contexts
[[gnu::noinline]] inline void WGccDebugBreak()
{
  __asm__ __volatile__("int3");
}
#    define W_DEBUG_BREAK \
      {                    \
        WGccDebugBreak(); \
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

#  define W_WARNING_PUSH() _Pragma("GCC diagnostic push")
#  define W_WARNING_POP() _Pragma("GCC diagnostic pop")
#  define W_WARNING_DISABLE_GCC(_x) _Pragma(W_PP_STRINGIFY(GCC diagnostic ignored _x))

#  define W_DECL_EXPORT [[gnu::visibility("default")]]
#  define W_DECL_IMPORT [[gnu::visibility("default")]]
#  define W_DECL_EXPORT_FRIEND
#  define W_DECL_IMPORT_FRIEND

#else

#  define W_WARNING_DISABLE_GCC(_x)

#endif
