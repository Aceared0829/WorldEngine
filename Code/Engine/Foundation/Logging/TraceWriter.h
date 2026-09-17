#pragma once

#include <Foundation/Logging/Log.h>

namespace WLogWriter
{
  /// Log writer that forwards log messages as trace events.
  ///
  /// Emits each log message as a "LogMessage" trace event with fields Type, Indentation, and Text.
  class W_FOUNDATION_DLL Tracing
  {
  public:
    /// Register this with WGlobalLog::AddLogWriter to forward all log messages as trace events.
    static void LogMessageHandler(const WLoggingEventData& eventData);
  };
} // namespace WLogWriter
