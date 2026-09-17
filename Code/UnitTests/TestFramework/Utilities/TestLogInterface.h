#pragma once
#include <Foundation/Logging/Log.h>
#include <TestFramework/TestFrameworkDLL.h>

/// An WLogInterface that expects and handles error messages during test runs. Can be
/// used to ensure that expected error messages are produced by the tested functionality.
/// Expected error messages are not passed on and do not cause tests to fail.
class W_TEST_DLL WTestLogInterface : public WLogInterface
{
public:
  WTestLogInterface() = default;
  ~WTestLogInterface();
  virtual void HandleLogMessage(const WLoggingEventData& le) override;

  /// Add expected message. Will fail the test when the expected message is not
  /// encountered. Can take an optional count, if messages are expected multiple times
  void ExpectMessage(const char* szMsg, WLogMsgType::Enum type = WLogMsgType::All, WInt32 iCount = 1);

  /// Set the log interface that unhandled messages are forwarded to.
  void SetParentLog(WLogInterface* pInterface) { m_pParentLog = pInterface; }

private:
  WLogInterface* m_pParentLog = nullptr;

  struct ExpectedMsg
  {
    WInt32 m_iCount = 0;
    WString m_sMsgSubString;
    WLogMsgType::Enum m_Type = WLogMsgType::All;
  };

  mutable WMutex m_Mutex;
  WHybridArray<ExpectedMsg, 8> m_ExpectedMessages;
};

/// A class that sets a custom WTestLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope. The test version passes the previous
/// WLogInterface on to the WTestLogInterface to enable passing on unhandled messages.
///
/// If bCatchMessagesGlobally is false, the system only intercepts messages on the current thread.
/// If bCatchMessagesGlobally is true, it will also intercept messages from other threads, as long as they
/// go through WGlobalLog. See WGlobalLog::SetGlobalLogOverride().
class W_TEST_DLL WTestLogSystemScope : public WLogSystemScope
{
public:
  explicit WTestLogSystemScope(WTestLogInterface* pInterface, bool bCatchMessagesGlobally = false)
    : WLogSystemScope(pInterface)
  {
    m_bCatchMessagesGlobally = bCatchMessagesGlobally;
    pInterface->SetParentLog(m_pPrevious);

    if (m_bCatchMessagesGlobally)
    {
      WGlobalLog::SetGlobalLogOverride(pInterface);
    }
  }

  ~WTestLogSystemScope()
  {
    if (m_bCatchMessagesGlobally)
    {
      WGlobalLog::SetGlobalLogOverride(nullptr);
    }
  }

private:
  bool m_bCatchMessagesGlobally = false;
};
