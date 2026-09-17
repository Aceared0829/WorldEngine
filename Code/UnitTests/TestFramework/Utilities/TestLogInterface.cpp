#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

WTestLogInterface::~WTestLogInterface()
{
  for (const ExpectedMsg& msg : m_ExpectedMessages)
  {
    WInt32 count = msg.m_iCount;
    W_TEST_BOOL_MSG(count == 0, "Message \"%s\" was logged %d times %s than expected.", msg.m_sMsgSubString.GetData(), count < 0 ? -count : count,
      count < 0 ? "more" : "less");
  }
}

void WTestLogInterface::HandleLogMessage(const WLoggingEventData& le)
{
  {
    // in case this interface is used with WTestLogSystemScope to override the WGlobalLog (see WGlobalLog::SetGlobalLogOverride)
    // it must be thread-safe
    W_LOCK(m_Mutex);

    for (ExpectedMsg& msg : m_ExpectedMessages)
    {
      if (msg.m_Type != WLogMsgType::All && le.m_EventType != msg.m_Type)
        continue;

      if (le.m_sText.FindSubString(msg.m_sMsgSubString))
      {
        --msg.m_iCount;

        // filter out error and warning messages entirely
        if (le.m_EventType >= WLogMsgType::ErrorMsg && le.m_EventType <= WLogMsgType::WarningMsg)
          return;

        // pass all other messages along to the parent log
        break;
      }
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void WTestLogInterface::ExpectMessage(const char* szMsg, WLogMsgType::Enum type /*= WLogMsgType::All*/, WInt32 iCount /*= 1*/)
{
  W_LOCK(m_Mutex);

  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  W_ASSERT_DEV(iCount >= 1, "Message needs to be expected at least once");

  ExpectedMsg& em = m_ExpectedMessages.ExpandAndGetRef();
  em.m_sMsgSubString = szMsg;
  em.m_iCount = iCount;
  em.m_Type = type;
}
