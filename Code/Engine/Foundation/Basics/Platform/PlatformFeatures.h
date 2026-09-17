#pragma once

// include the platform specific header
#include <Features_Platform.h>

#ifdef BUILDSYSTEM_ENABLE_GLFW_SUPPORT
#  define W_SUPPORTS_GLFW W_ON
#else
#  define W_SUPPORTS_GLFW W_OFF
#endif

// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef W_SUPPORTS_FILE_ITERATORS
#  error "W_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef W_USE_POSIX_FILE_API
#  error "W_USE_POSIX_FILE_API is not defined."
#endif

#ifndef W_SUPPORTS_FILE_STATS
#  error "W_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef W_SUPPORTS_MEMORY_MAPPED_FILE
#  error "W_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef W_SUPPORTS_SHARED_MEMORY
#  error "W_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef W_SUPPORTS_DYNAMIC_PLUGINS
#  error "W_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef W_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "W_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef W_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "W_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef W_SUPPORTS_LONG_PATHS
#  error "W_SUPPORTS_LONG_PATHS is not defined."
#endif

#ifndef W_SUPPORTS_IPC
#  error "W_SUPPORTS_IPC is not defined."
#endif

#if W_IS_NOT_EXCLUSIVE(W_PLATFORM_32BIT, W_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#if W_IS_NOT_EXCLUSIVE(W_PLATFORM_LITTLE_ENDIAN, W_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef W_MATH_CHECK_FOR_NAN
#  error "W_MATH_CHECK_FOR_NAN is not defined."
#endif

#if W_IS_NOT_EXCLUSIVE3(W_PLATFORM_ARCH_X86, W_PLATFORM_ARCH_ARM, W_PLATFORM_ARCH_WEB)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(W_SIMD_IMPLEMENTATION) || (W_SIMD_IMPLEMENTATION == 0)
#  error "W_SIMD_IMPLEMENTATION is not correctly defined."
#endif

#ifndef W_PLATFORM_NAME
#  error "W_PLATFORM_NAME is not defined."
#endif
