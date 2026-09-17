#pragma once

#include <Foundation/Logging/Log.h>

namespace WLogWriter
{

  /// A simple log writer that outputs all log messages to visual studios output window
  class W_FOUNDATION_DLL VisualStudio
  {
  public:
    /// Register this at WLog to write all log messages to visual studios output window.
    static void LogMessageHandler(const WLoggingEventData& eventData);
  };
} // namespace WLogWriter
