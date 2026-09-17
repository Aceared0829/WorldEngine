#pragma once

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Mutex.h>

namespace WLogWriter
{

  /// A log writer that writes out log messages to a plain text file.
  ///
  /// Create an instance of this class, register the LogMessageHandler at WLog and pass the pointer
  /// to the instance as the pPassThrough argument to it.
  ///
  /// In contrast to WLogWriter::HTML this writes through WOSFile, so it needs no data directories and
  /// can therefore already be active before the WFileSystem is configured. Writes are unbuffered, so the
  /// log is complete on disk even if the application crashes. That makes this the log writer to use for
  /// automated tests and tools that are inspected by another process.
  class W_FOUNDATION_DLL TextFile
  {
  public:
    ~TextFile();

    /// Register this at WLog to write all log messages to a text file.
    void LogMessageHandler(const WLoggingEventData& eventData);

    /// Opens the given file (an absolute path) for writing the log. From now on all incoming log messages are written into it.
    WResult BeginLog(WStringView sFile);

    /// Closes the file and stops logging the incoming messages.
    void EndLog();

    /// Whether BeginLog() succeeded and EndLog() hasn't been called yet.
    bool IsOpen() const;

    /// Allows to indicate in what form timestamps should be added to log messages.
    void SetTimestampMode(WLog::TimestampMode mode);

  private:
    mutable WMutex m_Mutex;
    WOSFile m_File;
    WLog::TimestampMode m_TimestampMode = WLog::TimestampMode::None;
  };
} // namespace WLogWriter
