#pragma once

#include <Foundation/Logging/Log.h>

namespace WLogWriter
{
  /// A simple log writer that writes out log messages using printf.
  class W_FOUNDATION_DLL Console
  {
  public:
    /// Register this at WLog to write all log messages to the console using printf.
    static void LogMessageHandler(const WLoggingEventData& eventData);

    /// Allows to indicate in what form timestamps should be added to log messages.
    static void SetTimestampMode(WLog::TimestampMode mode);

  private:
    static WLog::TimestampMode s_TimestampMode;
  };
} // namespace WLogWriter
