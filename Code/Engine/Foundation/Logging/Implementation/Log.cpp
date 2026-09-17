#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <stdarg.h>

#if TRACY_ENABLE
#  include <tracy/tracy/Tracy.hpp>
#endif

// Comment in to log into WLog::Print any message that is output while no logger is registered.
// #define DEBUG_STARTUP_LOGGING

WLogMsgType::Enum WLog::s_DefaultLogLevel = WLogMsgType::All;
WLog::PrintFunction WLog::s_CustomPrintFunction = nullptr;
WAtomicInteger32 WGlobalLog::s_uiMessageCount[WLogMsgType::ENUM_COUNT];
WLoggingEvent WGlobalLog::s_LoggingEvent;
WLogInterface* WGlobalLog::s_pOverrideLog = nullptr;
static thread_local bool s_bAllowOverrideLog = true;
static WMutex s_OverrideLogMutex;

/// The log system that messages are sent to when the user specifies no system himself.
static thread_local WLogInterface* s_DefaultLogSystem = nullptr;


WEventSubscriptionID WGlobalLog::AddLogWriter(WLoggingEvent::Handler handler)
{
  if (s_LoggingEvent.HasEventHandler(handler))
    return 0;

  return s_LoggingEvent.AddEventHandler(handler);
}

void WGlobalLog::RemoveLogWriter(WLoggingEvent::Handler handler)
{
  if (!s_LoggingEvent.HasEventHandler(handler))
    return;

  s_LoggingEvent.RemoveEventHandler(handler);
}

void WGlobalLog::RemoveLogWriter(WEventSubscriptionID& ref_subscriptionID)
{
  s_LoggingEvent.RemoveEventHandler(ref_subscriptionID);
}

void WGlobalLog::SetGlobalLogOverride(WLogInterface* pInterface)
{
  W_LOCK(s_OverrideLogMutex);

  W_ASSERT_DEV(pInterface == nullptr || s_pOverrideLog == nullptr, "Only one override log can be set at a time");
  s_pOverrideLog = pInterface;
}

void WGlobalLog::HandleLogMessage(const WLoggingEventData& le)
{
  if (s_pOverrideLog != nullptr && s_pOverrideLog != this && s_bAllowOverrideLog)
  {
    // only enter the lock when really necessary
    W_LOCK(s_OverrideLogMutex);

    // since s_bAllowOverrideLog is thread_local we do not need to re-check it

    // check this again under the lock, to be safe
    if (s_pOverrideLog != nullptr && s_pOverrideLog != this)
    {
      // disable the override log for the period in which it handles the event
      // to prevent infinite recursions
      s_bAllowOverrideLog = false;
      s_pOverrideLog->HandleLogMessage(le);
      s_bAllowOverrideLog = true;

      return;
    }
  }

  // else
  {
    const WLogMsgType::Enum ThisType = le.m_EventType;

    if ((ThisType > WLogMsgType::None) && (ThisType < WLogMsgType::All))
      s_uiMessageCount[ThisType].Increment();

#ifdef DEBUG_STARTUP_LOGGING
    if (s_LoggingEvent.IsEmpty())
    {
      WStringBuilder stmp = le.m_sText;
      stmp.Append("\n");
      WLog::Print(stmp);
    }
#endif
    s_LoggingEvent.Broadcast(le);
  }
}

WLogBlock::WLogBlock(WStringView sName, WStringView sContextInfo)
{
  m_pLogInterface = WLog::GetThreadLocalLogSystem();

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = WTime::Now().GetSeconds();
#endif
}


WLogBlock::WLogBlock(WLogInterface* pInterface, WStringView sName, WStringView sContextInfo)
{
  m_pLogInterface = pInterface;

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = WTime::Now().GetSeconds();
#endif
}

WLogBlock::~WLogBlock()
{
  if (!m_pLogInterface)
    return;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = WTime::Now().GetSeconds() - m_fSeconds;
#endif

  m_pLogInterface->m_pCurrentBlock = m_pParentBlock;

  WLog::EndLogBlock(m_pLogInterface, this);
}


void WLog::EndLogBlock(WLogInterface* pInterface, WLogBlock* pBlock)
{
  if (pBlock->m_bWritten)
  {
    WLoggingEventData le;
    le.m_EventType = WLogMsgType::EndGroup;
    le.m_sText = pBlock->m_sName;
    le.m_uiIndentation = pBlock->m_uiBlockDepth;
    le.m_sTag = pBlock->m_sContextInfo;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    le.m_fSeconds = pBlock->m_fSeconds;
#endif

    pInterface->HandleLogMessage(le);
  }
}

void WLog::WriteBlockHeader(WLogInterface* pInterface, WLogBlock* pBlock)
{
  if (!pBlock || pBlock->m_bWritten)
    return;

  pBlock->m_bWritten = true;

  WriteBlockHeader(pInterface, pBlock->m_pParentBlock);

  WLoggingEventData le;
  le.m_EventType = WLogMsgType::BeginGroup;
  le.m_sText = pBlock->m_sName;
  le.m_uiIndentation = pBlock->m_uiBlockDepth;
  le.m_sTag = pBlock->m_sContextInfo;

  pInterface->HandleLogMessage(le);
}

void WLog::BroadcastLoggingEvent(WLogInterface* pInterface, WLogMsgType::Enum type, WStringView sString)
{
  WLogBlock* pTopBlock = pInterface->m_pCurrentBlock;
  WUInt8 uiIndentation = 0;

  if (pTopBlock)
  {
    uiIndentation = pTopBlock->m_uiBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (sString.StartsWith("["))
  {
    const char* szAfterTag = sString.GetStartPointer();

    ++szAfterTag;

    WInt32 iPos = 0;

    // only treat it as a tag, if it is properly enclosed in square brackets and doesn't contain spaces
    while ((*szAfterTag != '\0') && (*szAfterTag != '[') && (*szAfterTag != ']') && (*szAfterTag != ' ') && (iPos < 31))
    {
      szTag[iPos] = *szAfterTag;
      ++szAfterTag;
      ++iPos;
    }

    if (*szAfterTag == ']')
    {
      szTag[iPos] = '\0';
      sString.SetStartPosition(szAfterTag + 1);
    }
    else
    {
      szTag[0] = '\0';
    }
  }

#if TRACY_ENABLE
  switch (type)
  {
    case WLogMsgType::ErrorMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::Red);
      break;
    case WLogMsgType::SeriousWarningMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::Orange);
      break;
    case WLogMsgType::WarningMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::Yellow);
      break;
    case WLogMsgType::SuccessMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::Green);
      break;
    case WLogMsgType::InfoMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::White);
      break;
    case WLogMsgType::DevMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::Grey);
      break;
    case WLogMsgType::DebugMsg:
      TracyMessageC(sString.GetStartPointer(), sString.GetElementCount(), tracy::Color::CornflowerBlue);
      break;

    default:
      break;
  }
#endif

  WLoggingEventData le;
  le.m_EventType = type;
  le.m_sText = sString;
  le.m_uiIndentation = uiIndentation;
  le.m_sTag = szTag;

  pInterface->HandleLogMessage(le);
  pInterface->m_uiLoggedMsgsSinceFlush++;
}

void WLog::Printf(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char buffer[4096];
  WStringUtils::vsnprintf(buffer, W_ARRAY_SIZE(buffer), szFormat, args);

  Print(buffer);

  va_end(args);
}

void WLog::SetCustomPrintFunction(PrintFunction func)
{
  s_CustomPrintFunction = func;
}

void WLog::GenerateFormattedTimestamp(TimestampMode mode, WStringBuilder& ref_sTimestampOut)
{
  // if mode is 'None', early out to not even retrieve a timestamp
  if (mode == TimestampMode::None)
  {
    return;
  }

  const WDateTime dateTime = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());

  switch (mode)
  {
    case TimestampMode::Numeric:
      ref_sTimestampOut.SetFormat("[{}] ", WArgDateTime(dateTime, WArgDateTime::ShowDate | WArgDateTime::ShowMilliseconds | WArgDateTime::ShowTimeZone));
      break;
    case TimestampMode::TimeOnly:
      ref_sTimestampOut.SetFormat("[{}] ", WArgDateTime(dateTime, WArgDateTime::ShowMilliseconds));
      break;
    case TimestampMode::Textual:
      ref_sTimestampOut.SetFormat(
        "[{}] ", WArgDateTime(dateTime, WArgDateTime::TextualDate | WArgDateTime::ShowMilliseconds | WArgDateTime::ShowTimeZone));
      break;
    default:
      W_ASSERT_DEV(false, "Unknown timestamp mode.");
      break;
  }
}

void WLog::SetThreadLocalLogSystem(WLogInterface* pInterface)
{
  W_ASSERT_DEV(pInterface != nullptr, "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

WLogInterface* WLog::GetThreadLocalLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
  {
    // use new, not W_DEFAULT_NEW, to prevent tracking
    s_DefaultLogSystem = new WGlobalLog;
  }

  return s_DefaultLogSystem;
}

void WLog::SetDefaultLogLevel(WLogMsgType::Enum logLevel)
{
  W_ASSERT_DEV(logLevel >= WLogMsgType::None && logLevel <= WLogMsgType::All, "Invalid default log level {}", (int)logLevel);

  s_DefaultLogLevel = logLevel;
}

WLogMsgType::Enum WLog::GetDefaultLogLevel()
{
  return s_DefaultLogLevel;
}

#define LOG_LEVEL_FILTER(MaxLevel)                                                                                                  \
  if (pInterface == nullptr)                                                                                                        \
    return;                                                                                                                         \
  if ((pInterface->GetLogLevel() == WLogMsgType::GlobalDefault ? WLog::s_DefaultLogLevel : pInterface->GetLogLevel()) < MaxLevel) \
    return;


void WLog::Error(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::ErrorMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::ErrorMsg, string.GetText(tmp));
}

void WLog::SeriousWarning(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::SeriousWarningMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void WLog::Warning(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::WarningMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::WarningMsg, string.GetText(tmp));
}

void WLog::Success(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::SuccessMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::SuccessMsg, string.GetText(tmp));
}

void WLog::Info(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::InfoMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::InfoMsg, string.GetText(tmp));
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)

void WLog::Dev(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::DevMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if W_ENABLED(W_COMPILE_FOR_DEBUG)

void WLog::Debug(WLogInterface* pInterface, const WFormatString& string)
{
  LOG_LEVEL_FILTER(WLogMsgType::DebugMsg);

  WStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, WLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

bool WLog::Flush(WUInt32 uiNumNewMsgThreshold, WTime timeIntervalThreshold, WLogInterface* pInterface /*= GetThreadLocalLogSystem()*/)
{
  if (pInterface == nullptr || pInterface->m_uiLoggedMsgsSinceFlush == 0) // if really nothing was logged, don't execute a flush
    return false;

  const WTime tNow = WTime::Now();

  if (pInterface->m_uiLoggedMsgsSinceFlush <= uiNumNewMsgThreshold && tNow - pInterface->m_LastFlushTime < timeIntervalThreshold)
    return false;

  BroadcastLoggingEvent(pInterface, WLogMsgType::Flush, nullptr);

  pInterface->m_uiLoggedMsgsSinceFlush = 0;
  pInterface->m_LastFlushTime = tNow;

  return true;
}
