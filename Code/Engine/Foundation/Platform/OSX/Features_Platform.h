#pragma once

#include <cstdio>
#include <pthread.h>
#include <sys/malloc.h>
#include <sys/time.h>

// unset common macros
#undef min
#undef max

#define W_PLATFORM_NAME "OSX"

#undef W_PLATFORM_OSX
#define W_PLATFORM_OSX W_ON

#undef W_PLATFORM_LITTLE_ENDIAN
#define W_PLATFORM_LITTLE_ENDIAN W_ON

#undef W_PLATFORM_PATH_SEPARATOR
#define W_PLATFORM_PATH_SEPARATOR '/'

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef W_USE_POSIX_FILE_API
#define W_USE_POSIX_FILE_API W_ON

/// Iterating through the file system is not supported
#undef W_SUPPORTS_FILE_ITERATORS
#define W_SUPPORTS_FILE_ITERATORS W_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef W_SUPPORTS_FILE_STATS
#define W_SUPPORTS_FILE_STATS W_ON

/// Directory watcher is not supported
#undef W_SUPPORTS_DIRECTORY_WATCHER
#define W_SUPPORTS_DIRECTORY_WATCHER W_OFF

/// Memory mapping a file is supported.
#undef W_SUPPORTS_MEMORY_MAPPED_FILE
#define W_SUPPORTS_MEMORY_MAPPED_FILE W_ON

/// Shared memory IPC is supported.
#undef W_SUPPORTS_SHARED_MEMORY
#define W_SUPPORTS_SHARED_MEMORY W_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef W_SUPPORTS_DYNAMIC_PLUGINS
#define W_SUPPORTS_DYNAMIC_PLUGINS W_OFF

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
#define W_SUPPORTS_IPC W_OFF

// SIMD support
#undef W_SIMD_IMPLEMENTATION
#define W_SIMD_IMPLEMENTATION W_SIMD_IMPLEMENTATION_FPU
