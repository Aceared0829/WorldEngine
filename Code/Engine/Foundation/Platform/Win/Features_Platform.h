#pragma once

/// \file

#define W_PLATFORM_NAME "Windows"

#undef W_PLATFORM_WINDOWS
#undef W_PLATFORM_WINDOWS_UWP
#undef W_PLATFORM_WINDOWS_DESKTOP

#define W_PLATFORM_WINDOWS W_ON
#define W_PLATFORM_WINDOWS_UWP W_OFF
#define W_PLATFORM_WINDOWS_DESKTOP W_ON

#undef W_PLATFORM_LITTLE_ENDIAN
#define W_PLATFORM_LITTLE_ENDIAN W_ON

#undef W_PLATFORM_PATH_SEPARATOR
#define W_PLATFORM_PATH_SEPARATOR '\\'

#ifdef _WIN64
#  undef W_PLATFORM_64BIT
#  define W_PLATFORM_64BIT W_ON
#else
#  undef W_PLATFORM_32BIT
#  define W_PLATFORM_32BIT W_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef W_USE_POSIX_FILE_API
#define W_USE_POSIX_FILE_API W_OFF

/// Iterating through the file system is supported
#undef W_SUPPORTS_FILE_ITERATORS
#define W_SUPPORTS_FILE_ITERATORS W_ON

/// Getting the stats of a file (modification times etc.) is supported.
#undef W_SUPPORTS_FILE_STATS
#define W_SUPPORTS_FILE_STATS W_ON

/// Directory watcher is supported on non UWP platforms.
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
#define W_SUPPORTS_CASE_INSENSITIVE_PATHS W_ON

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
#  if W_ENABLED(W_COMPILER_MSVC_CLANG)
#    define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_NEON
#  else
#    define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_FPU
#  endif
#else
#  error "Unknown architecture."
#endif

#undef W_SUPPORTS_CRASH_DUMPS
#define W_SUPPORTS_CRASH_DUMPS W_ON

#undef W_SUPPORTS_LONG_PATHS
#define W_SUPPORTS_LONG_PATHS W_ON
