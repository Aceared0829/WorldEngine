#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Status.h>

void WResult::AssertSuccess(const char* szMsg /*= nullptr*/, const char* szDetails /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (szMsg)
  {
    W_REPORT_FAILURE(szMsg, szDetails);
  }
  else
  {
    W_REPORT_FAILURE("An operation failed unexpectedly.");
  }
}

WStatus::WStatus(const WFormatString& fmt)
  : m_Result(W_FAILURE)
{
  WStringBuilder sMsg;
  m_sMessage = fmt.GetText(sMsg);
}

bool WStatus::LogFailure(WLogInterface* pLog) const
{
  if (Failed())
  {
    WLogInterface* pInterface = pLog ? pLog : WLog::GetThreadLocalLogSystem();
    WLog::Error(pInterface, "{0}", m_sMessage);
  }

  return Failed();
}

void WStatus::AssertSuccess(const char* szMsg /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (szMsg)
  {
    W_REPORT_FAILURE(szMsg, m_sMessage);
  }
  else
  {
    W_REPORT_FAILURE("An operation failed unexpectedly.", m_sMessage);
  }
}
