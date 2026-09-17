#pragma once

#if defined(__clang__) || defined(__GNUC__)

#  if defined(__x86_64__) || defined(__i386__)
#    undef W_PLATFORM_ARCH_X86
#    define W_PLATFORM_ARCH_X86 W_ON
#  elif defined(__arm__) || defined(__aarch64__)
#    undef W_PLATFORM_ARCH_ARM
#    define W_PLATFORM_ARCH_ARM W_ON
#  elif (__EMSCRIPTEN__)
#    undef W_PLATFORM_ARCH_WEB
#    define W_PLATFORM_ARCH_WEB W_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(__x86_64__) || defined(__aarch64__)
#    undef W_PLATFORM_64BIT
#    define W_PLATFORM_64BIT W_ON
#  elif defined(__i386__) || defined(__arm__)
#    undef W_PLATFORM_32BIT
#    define W_PLATFORM_32BIT W_ON
#  elif (__EMSCRIPTEN__)
#    undef W_PLATFORM_32BIT
#    define W_PLATFORM_32BIT W_ON
#  else
#    error unhandled platform bit count
#  endif

#elif defined(_MSC_VER)

#  if defined(_M_AMD64) || defined(_M_IX86)
#    undef W_PLATFORM_ARCH_X86
#    define W_PLATFORM_ARCH_X86 W_ON
#  elif defined(_M_ARM) || defined(_M_ARM64)
#    undef W_PLATFORM_ARCH_ARM
#    define W_PLATFORM_ARCH_ARM W_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(_M_AMD64) || defined(_M_ARM64)
#    undef W_PLATFORM_64BIT
#    define W_PLATFORM_64BIT W_ON
#  elif defined(_M_IX86) || defined(_M_ARM)
#    undef W_PLATFORM_32BIT
#    define W_PLATFORM_32BIT W_ON
#  else
#    error unhandled platform bit count
#  endif

#else
#  error unhandled compiler
#endif
