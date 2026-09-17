#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Time/Time.h>

/// Use this helper macro to easily create a scoped logging group. Will generate unique variable names to make the static code
/// analysis happy.
#define W_LOG_BLOCK WLogBlock W_PP_CONCAT(_logblock_, W_SOURCE_LINE)

/// Use this helper macro to easily mute all logging in a scope.
#define W_LOG_BLOCK_MUTE()                               \
  WMuteLog W_PP_CONCAT(_logmuteblock_, W_SOURCE_LINE); \
  WLogSystemScope W_PP_CONCAT(_logscope_, W_SOURCE_LINE)(&W_PP_CONCAT(_logmuteblock_, W_SOURCE_LINE))

// Forward declaration, class is at the end of this file
class WLogBlock;


/// Describes the types of events that WLog sends.
struct W_FOUNDATION_DLL WLogMsgType
{
  using StorageType = WInt8;

  enum Enum : WInt8
  {
    GlobalDefault = -4,    ///< Takes the log level from the WLog default value. See WLog::SetDefaultLogLevel().
    Flush = -3,            ///< The user explicitly called WLog::Flush() to instruct log writers to flush any cached output.
    BeginGroup = -2,       ///< A logging group has been opened.
    EndGroup = -1,         ///< A logging group has been closed.
    None = 0,              ///< Can be used to disable all log message types.
    ErrorMsg = 1,          ///< An error message.
    SeriousWarningMsg = 2, ///< A serious warning message.
    WarningMsg = 3,        ///< A warning message.
    SuccessMsg = 4,        ///< A success message.
    InfoMsg = 5,           ///< An info message.
    DevMsg = 6,            ///< A development message.
    DebugMsg = 7,          ///< A debug message.
    All = 8,               ///< Can be used to enable all log message types.
    ENUM_COUNT,
    Default = None,
  };
};

/// The data that is sent through WLogInterface.
struct W_FOUNDATION_DLL WLoggingEventData
{
  /// The type of information that is sent.
  WLogMsgType::Enum m_EventType = WLogMsgType::None;

  /// How many "levels" to indent.
  WUInt8 m_uiIndentation = 0;

  /// The information text.
  WStringView m_sText;

  /// An optional tag extracted from the log-string (if it started with "[SomeTag]Logging String.") Can be used by log-writers for
  /// additional configuration, or simply be ignored.
  WStringView m_sTag;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  /// Used by log-blocks for profiling the duration of the block
  double m_fSeconds = 0;
#endif
};

using WLoggingEvent = WEvent<const WLoggingEventData&, WMutex>;

/// Base class for all logging classes.
///
/// You can derive from this class to create your own logging system,
/// which you can pass to the functions in WLog.
class W_FOUNDATION_DLL WLogInterface
{
public:
  /// Override this function to handle logging events.
  virtual void HandleLogMessage(const WLoggingEventData& le) = 0;

  /// LogLevel is between WLogEventType::None and WLogEventType::All and defines which messages will be logged and which will be
  /// filtered out.
  W_ALWAYS_INLINE void SetLogLevel(WLogMsgType::Enum logLevel) { m_LogLevel = logLevel; }

  /// Returns the currently set log level.
  W_ALWAYS_INLINE WLogMsgType::Enum GetLogLevel() { return m_LogLevel; }

private:
  friend class WLog;
  friend class WLogBlock;
  WLogBlock* m_pCurrentBlock = nullptr;
  WLogMsgType::Enum m_LogLevel = WLogMsgType::GlobalDefault;
  WUInt32 m_uiLoggedMsgsSinceFlush = 0;
  WTime m_LastFlushTime;
};


/// Used to ignore all log messages.
/// \sa W_LOG_BLOCK_MUTE
class WMuteLog : public WLogInterface
{
public:
  WMuteLog()
  {
    SetLogLevel(WLogMsgType::None);
  }

  virtual void HandleLogMessage(const WLoggingEventData&) override {}
};


/// This is the standard log system that WLog sends all messages to.
///
/// It allows to register log writers, such that you can be informed of all log messages and write them
/// to different outputs.
class W_FOUNDATION_DLL WGlobalLog : public WLogInterface
{
public:
  virtual void HandleLogMessage(const WLoggingEventData& le) override;

  /// Allows to register a function as an event receiver.
  static WEventSubscriptionID AddLogWriter(WLoggingEvent::Handler handler);

  /// Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(WLoggingEvent::Handler handler);

  /// Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(WEventSubscriptionID& ref_subscriptionID);

  /// Returns how many message of the given type occurred.
  static WUInt32 GetMessageCount(WLogMsgType::Enum messageType) { return s_uiMessageCount[messageType]; }

  /// WLogInterfaces are thread_local and therefore a dedicated WGlobalLog is created per thread.
  /// Especially during testing one may want to replace the log system everywhere, to catch certain messages, no matter on which thread they
  /// happen. Unfortunately that is not so easy, as one cannot modify the thread_local system for all the other threads. This function makes
  /// it possible to at least force all messages that go through any WGlobalLog to be redirected to one other log interface. Be aware that
  /// that interface has to be thread-safe. Also, only one override can be set at a time, SetGlobalLogOverride() will assert that no other
  /// override is set at the moment.
  static void SetGlobalLogOverride(WLogInterface* pInterface);

private:
  /// Counts the number of messages of each type.
  static WAtomicInteger32 s_uiMessageCount[WLogMsgType::ENUM_COUNT];

  /// Manages all the Event Handlers for the logging events.
  static WLoggingEvent s_LoggingEvent;

  static WLogInterface* s_pOverrideLog;

private:
  W_DISALLOW_COPY_AND_ASSIGN(WGlobalLog);

  friend class WLog; // only WLog may create instances of this class
  WGlobalLog() = default;
};

/// Static class that allows to write out logging information.
///
/// This class takes logging information, prepares it and then broadcasts it to all interested code
/// via the event interface. It does not write anything on disk or somewhere else, itself. Instead it
/// allows to register custom log writers that can then write it to disk, to console, send it over a
/// network or pop up a message box. Whatever suits the current situation.
/// Since event handlers can be registered only temporarily, it is also possible to just gather all
/// errors that occur during some operation and then unregister the event handler again.
class W_FOUNDATION_DLL WLog
{
public:
  /// Allows to change which logging system is used by default on the current thread. If nothing is set, WGlobalLog is used.
  ///
  /// Replacing the log system on a thread does not delete the previous system, so it can be reinstated later again.
  /// This can be used to temporarily route all logging to a custom system.
  static void SetThreadLocalLogSystem(WLogInterface* pInterface);

  /// Returns the currently set default logging system, or a thread local instance of WGlobalLog, if nothing else was set.
  static WLogInterface* GetThreadLocalLogSystem();

  /// Sets the default log level which is used by all WLogInterface's that have their log level set to WLogMsgType::GlobalDefault
  static void SetDefaultLogLevel(WLogMsgType::Enum logLevel);

  /// Returns the currently set default log level.
  static WLogMsgType::Enum GetDefaultLogLevel();

  /// An error that needs to be fixed as soon as possible.
  static void Error(WLogInterface* pInterface, const WFormatString& string);

  /// An error that needs to be fixed as soon as possible.
  template <typename... ARGS>
  static void Error(WStringView sFormat, ARGS&&... args)
  {
    Error(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Error() to output messages to a specific log.
  template <typename... ARGS>
  static void Error(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Error(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(WLogInterface* pInterface, const WFormatString& string);

  /// Not an error, but definitely a big problem, that should be looked into very soon.
  template <typename... ARGS>
  static void SeriousWarning(WStringView sFormat, ARGS&&... args)
  {
    SeriousWarning(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of SeriousWarning() to output messages to a specific log.
  template <typename... ARGS>
  static void SeriousWarning(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    SeriousWarning(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(WLogInterface* pInterface, const WFormatString& string);

  /// A potential problem or a performance warning. Might be possible to ignore it.
  template <typename... ARGS>
  static void Warning(WStringView sFormat, ARGS&&... args)
  {
    Warning(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Warning() to output messages to a specific log.
  template <typename... ARGS>
  static void Warning(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Warning(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Status information that something was completed successfully.
  static void Success(WLogInterface* pInterface, const WFormatString& string);

  /// Status information that something was completed successfully.
  template <typename... ARGS>
  static void Success(WStringView sFormat, ARGS&&... args)
  {
    Success(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Success() to output messages to a specific log.
  template <typename... ARGS>
  static void Success(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Success(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Status information that is important.
  static void Info(WLogInterface* pInterface, const WFormatString& string);

  /// Status information that is important.
  template <typename... ARGS>
  static void Info(WStringView sFormat, ARGS&&... args)
  {
    Info(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Info() to output messages to a specific log.
  template <typename... ARGS>
  static void Info(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Info(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  static void Dev(WLogInterface* pInterface, const WFormatString& string);

  /// Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  template <typename... ARGS>
  static void Dev(WStringView sFormat, ARGS&&... args)
  {
    Dev(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Dev() to output messages to a specific log.
  template <typename... ARGS>
  static void Dev(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Dev(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  static void Debug(WLogInterface* pInterface, const WFormatString& string);

  /// Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  template <typename... ARGS>
  static void Debug(WStringView sFormat, ARGS&&... args)
  {
    Debug(GetThreadLocalLogSystem(), WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Overload of Debug() to output messages to a specific log.
  template <typename... ARGS>
  static void Debug(WLogInterface* pInterface, WStringView sFormat, ARGS&&... args)
  {
    Debug(pInterface, WFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Instructs log writers to flush their caches, to ensure all log output (even non-critical information) is written.
  ///
  /// On some log writers this has no effect.
  /// Do not call this too frequently as it incurs a performance penalty.
  ///
  /// \param uiNumNewMsgThreshold
  ///   If this is set to a number larger than zero, the flush may be ignored if the given WLogInterface
  ///   has logged fewer than this many messages since the last flush.
  /// \param timeIntervalThreshold
  ///   The flush may be ignored if less time has past than this, since the last flush.
  ///
  /// If either enough messages have been logged, or the flush interval has been exceeded, the flush is executed.
  /// To force a flush, set \a uiNumNewMsgThreshold  to zero.
  /// However, a flush is always ignored if not a single message was logged in between.
  ///
  /// \return Returns true if the flush is executed.
  static bool Flush(WUInt32 uiNumNewMsgThreshold = 0, WTime timeIntervalThreshold = WTime::MakeFromSeconds(10), WLogInterface* pInterface = GetThreadLocalLogSystem());

  /// Usually called internally by the other log functions, but can be called directly, if the message type is already known.
  /// pInterface must be != nullptr.
  static void BroadcastLoggingEvent(WLogInterface* pInterface, WLogMsgType::Enum type, WStringView sString);

  /// Calls low-level OS functionality to print a string to the typical outputs, e.g. printf and OutputDebugString.
  ///
  /// Use this function to log unrecoverable errors like asserts, crash handlers etc.
  /// This function is meant for short term debugging when actual printing to the console is desired. Code using it should be temporary.
  /// This function flushes the output immediately, to ensure output is never lost during a crash. Consequently it has a high performance
  /// overhead.
  static void Print(const char* szText);

  /// Calls low-level OS functionality to print a string to the typical outputs. Forwards to Print.
  /// \note This function uses actual printf formatting, not WFormatString syntax.
  /// \sa WLog::Print
  static void Printf(const char* szFormat, ...);

  /// Signature of the custom print function used by WLog::SetCustomPrintFunction.
  using PrintFunction = void (*)(const char* szText);

  /// Sets a custom function that is called in addition to the default behavior of WLog::Print.
  static void SetCustomPrintFunction(PrintFunction func);

  /// Shows a simple message box using the OS functionality.
  ///
  /// This should only be used for critical information that can't be conveyed in another way.
  static void OsMessageBox(const WFormatString& text);

  /// This enum is used in context of outputting timestamp information to indicate a formatting for said timestamps.
  enum class TimestampMode
  {
    None = 0,     ///< No timestamp will be added at all.
    Numeric = 1,  ///< A purely numeric timestamp will be added. Ex.: [2019-08-16 13:40:30.345 (UTC)] Log message.
    Textual = 2,  ///< A timestamp with textual fields will be added. Ex.: [2019 Aug 16 (Fri) 13:40:30.345 (UTC)] Log message.
    TimeOnly = 3, ///< A short timestamp (time only, no timezone indicator) is added. Ex: [13:40:30.345] Log message.
  };

  static void GenerateFormattedTimestamp(TimestampMode mode, WStringBuilder& ref_sTimestampOut);

private:
  // Needed to call 'EndLogBlock'
  friend class WLogBlock;

  /// Which messages to filter out by default.
  static WLogMsgType::Enum s_DefaultLogLevel;

  /// Ends grouping log messages.
  static void EndLogBlock(WLogInterface* pInterface, WLogBlock* pBlock);

  static void WriteBlockHeader(WLogInterface* pInterface, WLogBlock* pBlock);

  static PrintFunction s_CustomPrintFunction;
};


/// Instances of this class will group messages in a scoped block together.
class W_FOUNDATION_DLL WLogBlock
{
public:
  /// Creates a named grouping block for log messages.
  ///
  /// Use the szContextInfo to pass in a string that can give additional context information (e.g. a file name).
  /// This string must point to valid memory until after the log block object is destroyed.
  /// Log writers get these strings provided through the WLoggingEventData::m_szTag variable.
  /// \note The log block header (and context info) will not be printed until a message is successfully logged,
  /// i.e. as long as all messages in this block are filtered out (via the LogLevel setting), the log block
  /// header will not be printed, to prevent spamming the log.
  ///
  /// This constructor will output the log block data to the WGlobalLog.
  WLogBlock(WStringView sName, WStringView sContextInfo = {});

  /// Creates a named grouping block for log messages.
  ///
  /// This variant of the constructor takes an explicit WLogInterface to write the log messages to.
  WLogBlock(WLogInterface* pInterface, WStringView sName, WStringView sContextInfo = {});

  ~WLogBlock();

private:
  friend class WLog;

  WLogInterface* m_pLogInterface;
  WLogBlock* m_pParentBlock;
  WStringView m_sName;
  WStringView m_sContextInfo;
  WUInt8 m_uiBlockDepth;
  bool m_bWritten;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  double m_fSeconds; // for profiling
#endif
};

/// A class that sets a custom WLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope.
class W_FOUNDATION_DLL WLogSystemScope
{
public:
  /// The given WLogInterface is passed to WLog::SetThreadLocalLogSystem().
  explicit WLogSystemScope(WLogInterface* pInterface)
  {
    m_pPrevious = WLog::GetThreadLocalLogSystem();
    WLog::SetThreadLocalLogSystem(pInterface);
  }

  /// Resets the previous WLogInterface through WLog::SetThreadLocalLogSystem()
  ~WLogSystemScope() { WLog::SetThreadLocalLogSystem(m_pPrevious); }

protected:
  WLogInterface* m_pPrevious;

private:
  W_DISALLOW_COPY_AND_ASSIGN(WLogSystemScope);
};


/// A simple log interface implementation that gathers all messages in a string buffer.
class WLogSystemToBuffer : public WLogInterface
{
public:
  virtual void HandleLogMessage(const WLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case WLogMsgType::ErrorMsg:
        m_sBuffer.Append("Error: ", le.m_sText, "\n");
        break;
      case WLogMsgType::SeriousWarningMsg:
      case WLogMsgType::WarningMsg:
        m_sBuffer.Append("Warning: ", le.m_sText, "\n");
        break;
      case WLogMsgType::SuccessMsg:
      case WLogMsgType::InfoMsg:
      case WLogMsgType::DevMsg:
      case WLogMsgType::DebugMsg:
        m_sBuffer.Append(le.m_sText, "\n");
        break;
      default:
        break;
    }
  }

  WStringBuilder m_sBuffer;
};

#include <Foundation/Logging/Implementation/Log_inl.h>
