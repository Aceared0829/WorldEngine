#pragma once

#include <cstdio>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

// unset common macros
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#define W_PLATFORM_NAME "Linux"

#undef W_PLATFORM_LINUX
#define W_PLATFORM_LINUX W_ON

#undef W_PLATFORM_LITTLE_ENDIAN
#define W_PLATFORM_LITTLE_ENDIAN W_ON

#undef W_PLATFORM_PATH_SEPARATOR
#define W_PLATFORM_PATH_SEPARATOR '/'

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef W_USE_POSIX_FILE_API
#define W_USE_POSIX_FILE_API W_ON

/// If set to one linux posix extensions such as pipe2, dup3, etc are used.
#undef W_USE_LINUX_POSIX_EXTENSIONS
#define W_USE_LINUX_POSIX_EXTENSIONS W_ON

/// Iterating through the file system is not supported
#undef W_SUPPORTS_FILE_ITERATORS
#define W_SUPPORTS_FILE_ITERATORS W_ON

/// Getting the stats of a file (modification times etc.) is supported.
#undef W_SUPPORTS_FILE_STATS
#define W_SUPPORTS_FILE_STATS W_ON

/// Directory watcher is supported
#undef W_SUPPORTS_DIRECTORY_WATCHER
#define W_SUPPORTS_DIRECTORY_WATCHER W_ON

/// Memory mapping a file is supported.
#undef W_SUPPORTS_MEMORY_MAPPED_FILE
#define W_SUPPORTS_MEMORY_MAPPED_FILE W_ON

/// Shared memory IPC is supported.
#undef W_SUPPORTS_SHARED_MEMORY
#define W_SUPPORTS_SHARED_MEMORY W_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef W_SUPPORTS_DYNAMIC_PLUGINS
#define W_SUPPORTS_DYNAMIC_PLUGINS W_ON

/// Whether applications can access any file (not sandboxed)
#undef W_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define W_SUPPORTS_UNRESTRICTED_FILE_ACCESS W_ON

/// Whether file accesses can be done through paths that do not match exact casing
#undef W_SUPPORTS_CASE_INSENSITIVE_PATHS
#define W_SUPPORTS_CASE_INSENSITIVE_PATHS W_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef W_SUPPORTS_LONG_PATHS
#define W_SUPPORTS_LONG_PATHS W_ON

/// Whether starting other processes is supported.
#undef W_SUPPORTS_PROCESSES
#define W_SUPPORTS_PROCESSES W_ON

/// Whether inter-process communication via pipes is supported
#undef W_SUPPORTS_IPC
#define W_SUPPORTS_IPC W_ON

// SIMD support
#undef W_SIMD_IMPLEMENTATION

#if W_ENABLED(W_PLATFORM_ARCH_X86)
#  define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_SSE
#  if (BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL >= 50)
#    define W_SSE_LEVEL W_SSE_AVX
#  elif (BUILDSYSTEM_MIN_REQUIRED_SSE_LEVEL >= 41)
#    define W_SSE_LEVEL W_SSE_41
#  else
#    define W_SSE_LEVEL W_SSE_20
#  endif
#elif W_ENABLED(W_PLATFORM_ARCH_ARM)
#  define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_NEON
#else
#  error "Unknown architecture."
#endif

/// Crash dumps are supported on Linux.
#undef W_SUPPORTS_CRASH_DUMPS
#define W_SUPPORTS_CRASH_DUMPS W_ON
