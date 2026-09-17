#pragma once

#if defined(BUILDSYSTEM_ENABLE_PERFETTO_SUPPORT)

#  include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
  perfetto::Category("W")
    .SetDescription("WorldEngine trace events"));

class W_FOUNDATION_DLL WPerfettoRegistration
{
public:
  static void EnsureInitialized();
  static void Flush();
};

#  define W_TRACE_INTERNAL_FLUSH() WPerfettoRegistration::Flush()

#  define W_TRACE_INTERNAL_VALUE(FieldName, Value) FieldName, (Value)

#  define W_TRACE_LEVEL_TO_STRING_(Level)                                                   \
    ((Level) == WTraceLevel::Error ? "Error" : (Level) == WTraceLevel::Warning ? "Warning" \
                                              : (Level) == WTraceLevel::Info    ? "Info"    \
                                                                                 : "Verbose")

#  define W_TRACE_INTERNAL_EVENT(EventName, Level, ...)           \
    do                                                             \
    {                                                              \
      WPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_INSTANT("W", EventName,                         \
        "Level", W_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define W_TRACE_INTERNAL_SCOPE_BEGIN(EventName, Level, ...)     \
    do                                                             \
    {                                                              \
      WPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_BEGIN("W", EventName,                           \
        "Level", W_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define W_TRACE_INTERNAL_SCOPE_END(EventName) \
    TRACE_EVENT_END("W")

#  define W_TRACE_INTERNAL_ASYNC_BEGIN(EventName, Id, Level, ...) \
    do                                                             \
    {                                                              \
      WPerfettoRegistration::EnsureInitialized();                 \
      TRACE_EVENT_INSTANT("W", EventName,                         \
        perfetto::Flow::ProcessScoped(Id),                         \
        "Level", W_TRACE_LEVEL_TO_STRING_(Level), ##__VA_ARGS__); \
    } while (false)

#  define W_TRACE_INTERNAL_ASYNC_END(EventName, Id)                                      \
    do                                                                                    \
    {                                                                                     \
      TRACE_EVENT_INSTANT("W", EventName, perfetto::TerminatingFlow::ProcessScoped(Id)); \
    } while (false)

#else

// Perfetto support not available — redirect to no-op stubs.
#  include <Foundation/Platform/NoImpl/Tracing_NoImpl.h>

#endif
