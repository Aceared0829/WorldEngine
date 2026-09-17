#pragma once

/// \file

/// Used in conjunction with W_ENABLED and W_DISABLED for safe checks. Define something to W_ON or W_OFF to work with those macros.
#define W_ON =

/// Used in conjunction with W_ENABLED and W_DISABLED for safe checks. Define something to W_ON or W_OFF to work with those macros.
#define W_OFF !

/// Used in conjunction with W_ON and W_OFF for safe checks. Use #if W_ENABLED(x) or #if W_DISABLED(x) in conditional compilation.
#define W_ENABLED(x) (1 W_PP_CONCAT(x, =) 1)

/// Used in conjunction with W_ON and W_OFF for safe checks. Use #if W_ENABLED(x) or #if W_DISABLED(x) in conditional compilation.
#define W_DISABLED(x) (1 W_PP_CONCAT(x, =) 2)

/// Checks whether x AND y are both defined as W_ON or W_OFF. Usually used to check whether configurations overlap, to issue an error.
#define W_IS_NOT_EXCLUSIVE(x, y) ((1 W_PP_CONCAT(x, =) 1) == (1 W_PP_CONCAT(y, =) 1))

/// Checks that exactly one of x, y and z is defined as W_ON
#define W_IS_NOT_EXCLUSIVE3(x, y, z) ((W_ENABLED(x) + W_ENABLED(y) + W_ENABLED(z)) != 1)



// All the supported Platforms
#define W_PLATFORM_WINDOWS W_OFF         // enabled for all Windows platforms, both UWP and desktop
#define W_PLATFORM_WINDOWS_UWP W_OFF     // enabled for UWP apps, together with W_PLATFORM_WINDOWS
#define W_PLATFORM_WINDOWS_DESKTOP W_OFF // enabled for desktop apps, together with W_PLATFORM_WINDOWS
#define W_PLATFORM_OSX W_OFF
#define W_PLATFORM_LINUX W_OFF
#define W_PLATFORM_ANDROID W_OFF
#define W_PLATFORM_WEB W_OFF

// Different Bit OSes
#define W_PLATFORM_32BIT W_OFF
#define W_PLATFORM_64BIT W_OFF

// Different CPU architectures
#define W_PLATFORM_ARCH_X86 W_OFF
#define W_PLATFORM_ARCH_ARM W_OFF
#define W_PLATFORM_ARCH_WEB W_OFF

// Endianess
#define W_PLATFORM_LITTLE_ENDIAN W_OFF
#define W_PLATFORM_BIG_ENDIAN W_OFF

// Different Compilers
//
// W_COMPILER_MSVC means "MSVC compatible" (_MSC_VER is defined), which is also the case for clang-cl
// and for the clang driver when it targets *-windows-msvc. It can be used for things that any of those
// support, such as __declspec, __forceinline or the MSVC warning pragmas.
// Code that requires the MSVC front-end itself must use W_COMPILER_MSVC_PURE instead. This applies to
// MSVC-only intrinsics (for example the SVML functions _mm_exp_ps, _mm_div_epi32) and to the layout of
// the MSVC SIMD types (__m128::m128_f32 etc), none of which exist in clang.
#define W_COMPILER_MSVC W_OFF
#define W_COMPILER_MSVC_CLANG W_OFF // Clang front-end with MSVC compatibility
#define W_COMPILER_MSVC_PURE W_OFF  // MSVC front-end and CodeGen, no mixed compilers
#define W_COMPILER_CLANG W_OFF      // Clang front-end in GCC mode, NOT set for clang-cl (see W_COMPILER_MSVC_CLANG)
#define W_COMPILER_GCC W_OFF

// How to compile the engine
#define W_COMPILE_ENGINE_AS_DLL W_OFF
#define W_COMPILE_FOR_DEBUG W_OFF
#define W_COMPILE_FOR_DEVELOPMENT W_OFF

// Platform Features
#define W_USE_POSIX_FILE_API W_OFF
#define W_USE_LINUX_POSIX_EXTENSIONS W_OFF // linux specific posix extensions like pipe2, dup3, etc.
#define W_USE_CPP20_OPERATORS W_OFF
#define W_SUPPORTS_FILE_ITERATORS W_OFF
#define W_SUPPORTS_FILE_STATS W_OFF
#define W_SUPPORTS_DIRECTORY_WATCHER W_OFF
#define W_SUPPORTS_MEMORY_MAPPED_FILE W_OFF
#define W_SUPPORTS_SHARED_MEMORY W_OFF
#define W_SUPPORTS_DYNAMIC_PLUGINS W_OFF
#define W_SUPPORTS_UNRESTRICTED_FILE_ACCESS W_OFF
#define W_SUPPORTS_CASE_INSENSITIVE_PATHS W_OFF
#define W_SUPPORTS_CRASH_DUMPS W_OFF
#define W_SUPPORTS_LONG_PATHS W_OFF
#define W_SUPPORTS_IPC W_OFF

// Allocators
#define W_ALLOC_GUARD_ALLOCATIONS W_OFF
#define W_ALLOC_TRACKING_DEFAULT WAllocatorTrackingMode::Nothing

// Other Features
#define W_USE_PROFILING W_OFF
#define W_USE_TRACING W_OFF
#define W_USE_STRING_VALIDATION W_OFF

// Hashed String
/// Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled
/// by default.
#define W_HASHED_STRING_REF_COUNTING W_OFF

// Math Debug Checks
#define W_MATH_CHECK_FOR_NAN W_OFF

// SIMD support
#define W_SIMD_IMPLEMENTATION_FPU 1
#define W_SIMD_IMPLEMENTATION_SSE 2
#define W_SIMD_IMPLEMENTATION_NEON 3


// SSE levels
#define W_SSE_20 0x20
#define W_SSE_30 0x30
#define W_SSE_31 0x31
#define W_SSE_41 0x41
#define W_SSE_42 0x42
#define W_SSE_AVX 0x50
#define W_SSE_AVX2 0x51

#define W_SIMD_IMPLEMENTATION 0

// Application entry point code injection (undef and redefine in UserConfig.h if needed)
#define W_APPLICATION_ENTRY_POINT_CODE_INJECTION

// Interoperability with other libraries
#define W_INTEROP_STL_STRINGS W_OFF
#define W_INTEROP_STL_SPAN W_OFF
