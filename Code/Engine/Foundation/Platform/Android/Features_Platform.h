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

#define W_PLATFORM_NAME "Android"

#undef W_PLATFORM_ANDROID
#define W_PLATFORM_ANDROID W_ON

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
#define W_SUPPORTS_FILE_ITERATORS W_OFF

/// Directory watcher is not supported
#undef W_SUPPORTS_DIRECTORY_WATCHER
#define W_SUPPORTS_DIRECTORY_WATCHER W_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef W_SUPPORTS_FILE_STATS
#define W_SUPPORTS_FILE_STATS W_ON

/// Memory mapping a file is supported.
#undef W_SUPPORTS_MEMORY_MAPPED_FILE
#define W_SUPPORTS_MEMORY_MAPPED_FILE W_ON

/// Shared memory IPC is not supported.
/// shm_open / shm_unlink deprecated.
/// There is an alternative in ASharedMemory_create but that is only
/// available in API 26 upwards.
/// Could be implemented via JNI which defeats the purpose of a fast IPC channel
/// or we could just use an actual file as the shared memory block.
#undef W_SUPPORTS_SHARED_MEMORY
#define W_SUPPORTS_SHARED_MEMORY W_OFF

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef W_SUPPORTS_DYNAMIC_PLUGINS
#define W_SUPPORTS_DYNAMIC_PLUGINS W_OFF

/// Whether applications can access any file (not sandboxed)
#undef W_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define W_SUPPORTS_UNRESTRICTED_FILE_ACCESS W_OFF

/// Whether file accesses can be done through paths that do not match exact casing
#undef W_SUPPORTS_CASE_INSENSITIVE_PATHS
#define W_SUPPORTS_CASE_INSENSITIVE_PATHS W_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef W_SUPPORTS_LONG_PATHS
#define W_SUPPORTS_LONG_PATHS W_ON

/// Whether starting other processes is supported.
#undef W_SUPPORTS_PROCESSES
#define W_SUPPORTS_PROCESSES W_OFF

// SIMD support
#undef W_SIMD_IMPLEMENTATION

#if W_ENABLED(W_PLATFORM_ARCH_X86)
// Disabling SSE in emulator to increase FPU test coverage. Uncomment code below if performance is an issue for you.
// #  if __SSE4_1__ && __SSSE3__
// #    define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_SSE
// #    define W_SSE_LEVEL W_SSE_41
// #  else
#  define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_FPU
// #  endif
#elif W_ENABLED(W_PLATFORM_ARCH_ARM)
#  if W_ENABLED(W_PLATFORM_64BIT)
#    define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_NEON
#  else
#    define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_FPU
#  endif
#else
#  error "Unknown architecture."
#endif
