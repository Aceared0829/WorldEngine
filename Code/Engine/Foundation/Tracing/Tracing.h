#pragma once

#include <Foundation/Types/ScopeExit.h>

/// \file
/// Cross-platform tracing system for emitting structured events to the OS tracing infrastructure.
///
/// This header provides macros to emit instant events, scoped events, and async activities that are captured by ETW (Windows), LTTNG (Linux), or Perfetto (Android). Unlike W_PROFILE_SCOPE, which stores data in an in-process ring buffer for later JSON export, these macros send events to the operating system's tracing subsystem for capture by external tools.
///
/// Tracing is controlled by the W_USE_TRACING feature toggle (see UserConfig.h). When disabled, all macros expand to nothing with zero overhead.
///
/// \section tracing_usage Emitting Trace Events
///
/// \code{.cpp}
///   // Instant event (point-in-time)
///   W_TRACE_EVENT("TextureLoaded", WTraceLevel::Info,
///     W_TRACE_VALUE("Path", szTexturePath),
///     W_TRACE_VALUE("Width", uiWidth));
///
///   // Scoped event (RAII begin + end, measures duration)
///   {
///     W_TRACE_SCOPE("PhysicsStep", WTraceLevel::Verbose,
///       W_TRACE_VALUE("NumBodies", uiBodyCount));
///     // ... physics work ...
///   } // end event emitted automatically
///
///   // Async activity (correlated across threads by ID)
///   W_TRACE_ASYNC_BEGIN("AsyncLoad", uiRequestId, WTraceLevel::Info,
///     W_TRACE_VALUE("Resource", szPath));
///   // ... on another thread or later ...
///   W_TRACE_ASYNC_END("AsyncLoad", uiRequestId);
/// \endcode
///
/// \section tracing_provider Provider Setup
///
/// Each library or executable that emits trace events needs its own provider. Create two files in a Tracing/ subfolder:
///
/// \code{.cpp}
///   // MyLibrary/Tracing/TraceProvider.h
///   #pragma once
///   #include <Foundation/Tracing/Tracing.h>
///   W_DECLARE_TRACE_PROVIDER(g_WTrace_MyLibrary);
///   #define W_TRACE_PROVIDER g_WTrace_MyLibrary
/// \endcode
///
/// \code{.cpp}
///   // MyLibrary/Tracing/TraceProvider.cpp
///   #include <MyLibrary/MyLibraryPCH.h>
///   #include <MyLibrary/Tracing/TraceProvider.h>
///   W_IMPLEMENT_TRACE_PROVIDER(g_WTrace_MyLibrary, "W_MyLibrary");
/// \endcode
///
/// Source files that emit trace events include the provider header instead of Tracing.h:
/// \code{.cpp}
///   #include <MyLibrary/Tracing/TraceProvider.h>
///   void MyFunction()
///   {
///     W_TRACE_EVENT("MyEvent", WTraceLevel::Info, W_TRACE_VALUE("Key", 42));
///   }
/// \endcode
///
/// \section tracing_capture Capturing Traces
///
/// Use the cross-platform PowerShell 7 script at Utilities/Tracing/Capture-Trace.ps1. See that script for full documentation on per-platform setup, prerequisites, and manual steps.
///
/// \code{.sh}
///   # Interactive capture: Starts a trace and waits for a key press to stop the trace.
///   pwsh Utilities/Tracing/Capture-Trace.ps1
///
///   # Non-interactive:
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Start
///   # ... run application ...
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Stop -OutputPath trace
///
///   # Android (Perfetto) from any host:
///   pwsh Utilities/Tracing/Capture-Trace.ps1 -Android
/// \endcode
///
/// \sa W_TRACE_EVENT, W_TRACE_SCOPE, W_TRACE_ASYNC_BEGIN, W_TRACE_ASYNC_END, W_TRACE_VALUE

#include <Foundation/Basics.h>

/// Trace event severity levels.
///
/// These are WorldEngine's own levels, mapped to ETW / LTTNG / Perfetto by the platform backend.
struct WTraceLevel
{
  enum Enum : WUInt8
  {
    Error,   ///< Serious failure.
    Warning, ///< Potential problem.
    Info,    ///< Informational.
    Verbose, ///< Detailed diagnostic.
  };
};

#if W_ENABLED(W_USE_TRACING)
#  include <Tracing_Platform.h>
#endif

// Ensure provider macros are always defined (no-op when tracing is off or unsupported).
#ifndef W_DECLARE_TRACE_PROVIDER
#  define W_DECLARE_TRACE_PROVIDER(ProviderSymbol)
#endif

#ifndef W_IMPLEMENT_TRACE_PROVIDER
#  define W_IMPLEMENT_TRACE_PROVIDER(ProviderSymbol, ProviderName)
#endif

#if W_ENABLED(W_USE_TRACING) || defined(W_DOCS)

/// Field descriptor macro for use inside W_TRACE_EVENT, W_TRACE_SCOPE, and W_TRACE_ASYNC_BEGIN. Auto-detects the C++ type of Value. Cast the value if the auto-detection picks the wrong type.
#  define W_TRACE_VALUE(FieldName, Value) \
    W_TRACE_INTERNAL_VALUE(FieldName, Value)

/// Fires a single instant event. The Level parameter is an WTraceLevel::Enum.
///
/// Usage:
/// \code
///   W_TRACE_EVENT("CollisionDetected", WTraceLevel::Info,
///     W_TRACE_VALUE("NumContacts", iContacts),
///     W_TRACE_VALUE("Force", fImpact));
/// \endcode
///
/// \sa W_TRACE_SCOPE
#  define W_TRACE_EVENT(EventName, Level, ...) \
    W_TRACE_INTERNAL_EVENT(EventName, Level, ##__VA_ARGS__)

/// Opens a scoped trace that records begin time at construction and end time at destruction.
///
/// Fields are attached to the begin event. The Level parameter is an WTraceLevel::Enum.
///
/// Usage:
/// \code
///   {
///     W_TRACE_SCOPE("DrawCalls", WTraceLevel::Info,
///       W_TRACE_VALUE("BatchCount", uiBatches));
///     // ... work ...
///   }
/// \endcode
///
/// \sa W_TRACE_EVENT, W_TRACE_ASYNC_BEGIN, W_TRACE_SCOPE_BEGIN
#  define W_TRACE_SCOPE(EventName, Level, ...)                     \
    W_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ##__VA_ARGS__); \
    W_SCOPE_EXIT(W_TRACE_INTERNAL_SCOPE_END(EventName));

/// Manually begins a scoped trace event.
///
/// Unlike W_TRACE_SCOPE, this does not automatically emit an end event at scope exit. You must call W_TRACE_SCOPE_END with the same EventName when the region is complete. Use this when the begin and end do not fall within the same C++ scope (e.g. across callbacks).
///
/// \sa W_TRACE_SCOPE_END, W_TRACE_SCOPE
#  define W_TRACE_SCOPE_BEGIN(EventName, Level, ...) \
    W_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ##__VA_ARGS__)

/// Ends a scoped trace event previously started with W_TRACE_SCOPE_BEGIN.
///
/// Must use the same EventName that was passed to W_TRACE_SCOPE_BEGIN.
///
/// \sa W_TRACE_SCOPE_BEGIN
#  define W_TRACE_SCOPE_END(EventName) \
    W_TRACE_INTERNAL_SCOPE_END(EventName)

/// Begin a named async activity tied to an ID (WUInt64). The ID correlates begin/end across threads.
///
/// Usage:
/// \code
///   W_TRACE_ASYNC_BEGIN("AsyncLoad", uiRequestId, WTraceLevel::Info,
///     W_TRACE_VALUE("Resource", szPath));
///   // ... on another thread or later ...
///   W_TRACE_ASYNC_END("AsyncLoad", uiRequestId);
/// \endcode
///
/// \sa W_TRACE_ASYNC_END
#  define W_TRACE_ASYNC_BEGIN(EventName, Id, Level, ...) \
    W_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ##__VA_ARGS__)

/// End the async activity started with W_TRACE_ASYNC_BEGIN. Must use the same EventName and Id.
///
/// \sa W_TRACE_ASYNC_BEGIN
#  define W_TRACE_ASYNC_END(EventName, Id) \
    W_TRACE_INTERNAL_ASYNC_END(EventName, Id)

/// Flushes any buffered trace events to the OS tracing backend.
///
/// Useful before stopping a trace session to ensure all emitted events are captured.
#  ifdef W_TRACE_INTERNAL_FLUSH
#    define W_TRACE_FLUSH() W_TRACE_INTERNAL_FLUSH()
#  else
#    define W_TRACE_FLUSH()
#  endif

#else

#  define W_TRACE_VALUE(FieldName, Value)
#  define W_TRACE_EVENT(EventName, Level, ...)
#  define W_TRACE_SCOPE(EventName, Level, ...)
#  define W_TRACE_SCOPE_BEGIN(EventName, Level, ...)
#  define W_TRACE_SCOPE_END(EventName)
#  define W_TRACE_ASYNC_BEGIN(EventName, Id, Level, ...)
#  define W_TRACE_ASYNC_END(EventName, Id)
#  define W_TRACE_FLUSH()

#endif
