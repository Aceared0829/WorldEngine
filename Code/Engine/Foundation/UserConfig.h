#pragma once

/// \file

/// Global settings for how to to compile W.
/// Modify these settings as you needed in your project.


#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef W_COMPILE_ENGINE_AS_DLL
#  define W_COMPILE_ENGINE_AS_DLL W_ON
#else
#  undef W_COMPILE_ENGINE_AS_DLL
#  define W_COMPILE_ENGINE_AS_DLL W_OFF
#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Shipping)

// Development checks like assert.
#  undef W_COMPILE_FOR_DEVELOPMENT
#  define W_COMPILE_FOR_DEVELOPMENT W_OFF

// Performance profiling features
#  undef W_USE_PROFILING
#  define W_USE_PROFILING W_OFF

// OS tracing features (ETW / LTTNG / Perfetto)
#  undef W_USE_TRACING
#  define W_USE_TRACING W_OFF

// Tracking of memory allocations.
#  undef W_ALLOC_TRACKING_DEFAULT
#  define W_ALLOC_TRACKING_DEFAULT WAllocatorTrackingMode::Nothing

#else

// Development checks like assert.
#  undef W_COMPILE_FOR_DEVELOPMENT
#  define W_COMPILE_FOR_DEVELOPMENT W_ON

// Performance profiling features
#  undef W_USE_PROFILING
#  define W_USE_PROFILING W_ON

// OS tracing features (ETW / LTTNG / Perfetto)
#  undef W_USE_TRACING
#  define W_USE_TRACING W_ON

// Tracking of memory allocations.
#  undef W_ALLOC_TRACKING_DEFAULT

#  if W_ENABLED(W_PLATFORM_ANDROID)
#    define W_ALLOC_TRACKING_DEFAULT WAllocatorTrackingMode::AllocationStatsIgnoreLeaks
#  else
#    define W_ALLOC_TRACKING_DEFAULT WAllocatorTrackingMode::AllocationStatsAndStacktraces
#  endif

#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Debug)
#  undef W_MATH_CHECK_FOR_NAN
#  define W_MATH_CHECK_FOR_NAN W_ON
#  undef W_USE_STRING_VALIDATION
#  define W_USE_STRING_VALIDATION W_ON
#endif


/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define W_GAMEOBJECT_VELOCITY W_ON
