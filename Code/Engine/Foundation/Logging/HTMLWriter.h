#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>

namespace WLogWriter
{

  /// A log writer that writes out log messages to an HTML file.
  ///
  /// Create an instance of this class, register the LogMessageHandler at WLog and pass the pointer
  /// to the instance as the pPassThrough argument to it.
  class W_FOUNDATION_DLL HTML
  {
  public:
    ~HTML();

    /// Register this at WLog to write all log messages to an HTML file.
    void LogMessageHandler(const WLoggingEventData& eventData);

    /// Opens the given file for writing the log. From now on all incoming log messages are written into it.
    void BeginLog(WStringView sFile, WStringView sAppTitle);

    /// Closes the HTML file and stops logging the incoming message.
    void EndLog();

    /// Returns the name of the log-file that was really opened. Might be slightly different than what was given to BeginLog, to allow parallel
    /// execution of the same application.
    const WFileWriter& GetOpenedLogFile() const;

    /// Allows to indicate in what form timestamps should be added to log messages.
    void SetTimestampMode(WLog::TimestampMode mode);

  private:
    void WriteString(WStringView sText, WUInt32 uiColor);

    WFileWriter m_File;

    WLog::TimestampMode m_TimestampMode = WLog::TimestampMode::None;
  };
} // namespace WLogWriter
