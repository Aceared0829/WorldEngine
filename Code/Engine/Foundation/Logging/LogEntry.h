#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WLogMsgType);

/// A persistent log entry created from a WLoggingEventData.
/// Allows for a log event to survive for longer than just the event
/// and is reflected, allowing for it to be sent to remote targets.
struct W_FOUNDATION_DLL WLogEntry
{
  WLogEntry();
  WLogEntry(const WLoggingEventData& le);

  WString m_sMsg;
  WString m_sTag;
  WEnum<WLogMsgType> m_Type;
  WUInt8 m_uiIndentation = 0;
  double m_fSeconds = 0;
};

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WLogEntry);

/// A log interface implementation that converts a log event into
/// a WLogEntry and calls a delegate with it.
///
/// A typical use case is to re-route and store log messages in a scope:
/// \code{.cpp}
///   {
///     WLogEntryDelegate logger(([&array](WLogEntry& entry) -> void
///     {
///       array.PushBack(std::move(entry));
///     }));
///     WLogSystemScope logScope(&logger);
///     *log something*
///   }
/// \endcode
class W_FOUNDATION_DLL WLogEntryDelegate : public WLogInterface
{
public:
  using Callback = WDelegate<void(WLogEntry&)>;
  /// Log events will be delegated to the given callback.
  WLogEntryDelegate(Callback callback, WLogMsgType::Enum logLevel = WLogMsgType::All);
  virtual void HandleLogMessage(const WLoggingEventData& le) override;

private:
  Callback m_Callback;
};
