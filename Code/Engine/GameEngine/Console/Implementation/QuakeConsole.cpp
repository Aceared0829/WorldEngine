#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Console/LuaInterpreter.h>
#include <GameEngine/Console/QuakeConsole.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WQuakeConsole::WQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(W_DEFAULT_NEW(WCommandInterpreterLua));
#endif
}

WQuakeConsole::~WQuakeConsole()
{
  EnableLogOutput(false);
}

void WQuakeConsole::AddConsoleString(WStringView sText, WConsoleString::Type type)
{
  W_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  WConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = sText;
  cs.m_Type = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);

  WConsole::AddConsoleString(sText, type);
}

const WDeque<WConsoleString>& WQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void WQuakeConsole::LogHandler(const WLoggingEventData& data)
{
  WConsoleString::Type type = WConsoleString::Type::Default;

  switch (data.m_EventType)
  {
    case WLogMsgType::GlobalDefault:
    case WLogMsgType::Flush:
    case WLogMsgType::BeginGroup:
    case WLogMsgType::EndGroup:
    case WLogMsgType::None:
    case WLogMsgType::ENUM_COUNT:
    case WLogMsgType::All:
      return;

    case WLogMsgType::ErrorMsg:
      type = WConsoleString::Type::Error;
      break;

    case WLogMsgType::SeriousWarningMsg:
      type = WConsoleString::Type::SeriousWarning;
      break;

    case WLogMsgType::WarningMsg:
      type = WConsoleString::Type::Warning;
      break;

    case WLogMsgType::SuccessMsg:
      type = WConsoleString::Type::Success;
      break;

    case WLogMsgType::InfoMsg:
      type = WConsoleString::Type::Default;
      break;

    case WLogMsgType::DevMsg:
      type = WConsoleString::Type::Dev;
      break;

    case WLogMsgType::DebugMsg:
      type = WConsoleString::Type::Debug;
      break;
  }

  WStringBuilder sFormat;
  sFormat.SetPrintf("%*s", data.m_uiIndentation, "");
  sFormat.Append(data.m_sText);

  AddConsoleString(sFormat.GetData(), type);
}

void WQuakeConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    WStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
}

void WQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    WGlobalLog::AddLogWriter(WMakeDelegate(&WQuakeConsole::LogHandler, this));
  }
  else
  {
    WGlobalLog::RemoveLogWriter(WMakeDelegate(&WQuakeConsole::LogHandler, this));
  }
}

void WQuakeConsole::SaveState(WStreamWriter& inout_stream) const
{
  W_LOCK(m_Mutex);

  const WUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_InputHistory.GetCount();
  for (WUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    inout_stream << m_InputHistory[i];
  }

  inout_stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }
}

void WQuakeConsole::LoadState(WStreamReader& inout_stream)
{
  W_LOCK(m_Mutex);

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion > 100)
    return;

  if (uiVersion == 1)
  {
    WUInt32 count = 0;
    inout_stream >> count;
    m_InputHistory.SetCount(count);

    for (WUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      inout_stream >> m_InputHistory[i];
    }

    inout_stream >> count;

    WString sKey;
    WString sValue;

    for (WUInt32 i = 0; i < count; ++i)
    {
      inout_stream >> sKey;
      inout_stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}

void WQuakeConsole::ExecuteCommand(WStringView sInput)
{
  const bool bBind = sInput.StartsWith_NoCase("bind ");
  const bool bUnbind = sInput.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    WStringBuilder tmp;
    const char* szAfterCmd = WStringUtils::FindWordEnd(sInput.GetData(tmp), WStringUtils::IsWhiteSpace);              // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = WStringUtils::SkipCharacters(szAfterCmd, WStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = WStringUtils::FindWordEnd(szKeyNameStart, WStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    WStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey;                                                                                                         // copy the word into a zero terminated string

    const char* szCommandToBind = WStringUtils::SkipCharacters(szKeyNameEnd, WStringUtils::IsWhiteSpace);

    if (bUnbind || WStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  WConsole::ExecuteCommand(sInput);
}

void WQuakeConsole::BindKey(WStringView sKey, WStringView sCommand)
{
  WStringBuilder s;
  s.SetFormat("Binding key '{0}' to command '{1}'", sKey, sCommand);
  AddConsoleString(s, WConsoleString::Type::Success);

  m_BoundKeys[sKey] = sCommand;
}

void WQuakeConsole::UnbindKey(WStringView sKey)
{
  WStringBuilder s;
  s.SetFormat("Unbinding key '{0}'", sKey);
  AddConsoleString(s, WConsoleString::Type::Success);

  m_BoundKeys.Remove(sKey);
}

void WQuakeConsole::ExecuteBoundKey(WStringView sKey)
{
  auto it = m_BoundKeys.Find(sKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value());
  }
}

bool WQuakeConsole::ProcessInputCharacter(WUInt32 uiChar)
{
  switch (uiChar)
  {
    case 27: // Escape
      ClearInputLine();
      return false;

    case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
    }
      return false;

    case '\t':
      if (AutoComplete(m_sInputLine))
      {
        MoveCaret(500);
      }
      return false;

    case 13: // Enter
      AddToInputHistory(m_sInputLine);
      ExecuteCommand(m_sInputLine);
      ClearInputLine();
      return false;
  }

  return true;
}

bool WQuakeConsole::FilterInputCharacter(WUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void WQuakeConsole::ClampCaretPosition()
{
  m_iCaretPosition = WMath::Clamp<WInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void WQuakeConsole::MoveCaret(WInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void WQuakeConsole::Scroll(WInt32 iLines)
{
  if (m_bUseFilteredStrings)
    m_iScrollPosition = WMath::Clamp<WInt32>(m_iScrollPosition + iLines, 0, WMath::Max<WInt32>(m_FilteredConsoleStrings.GetCount() - 10, 0));
  else
    m_iScrollPosition = WMath::Clamp<WInt32>(m_iScrollPosition + iLines, 0, WMath::Max<WInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void WQuakeConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition = 0;
  m_iScrollPosition = 0;
  m_iCurrentInputHistoryElement = -1;

  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;

  InputStringChanged();
}

void WQuakeConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;
  m_iScrollPosition = 0;
}

void WQuakeConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void WQuakeConsole::RemoveCharacter(WUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());

  InputStringChanged();
}

void WQuakeConsole::AddInputCharacter(WUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  WUInt32 uiString[2] = {uiChar, 0};

  m_sInputLine.Insert(it.GetData(), WStringUtf8(uiString).GetData());

  MoveCaret(1);

  InputStringChanged();
}

void WQuakeConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    WInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyLeft;
    WInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyRight;
    WInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyHome;
    WInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyEnd;
    WInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyDelete;
    WInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyPageUp;
    WInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyPageDown;
    WInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyUp;
    WInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyDown;
    WInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyF2;
    WInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyF3;
    WInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (WInputManager::GetInputActionState("Console", "MoveCaretLeft") == WKeyState::Pressed)
      MoveCaret(-1);
    if (WInputManager::GetInputActionState("Console", "MoveCaretRight") == WKeyState::Pressed)
      MoveCaret(1);
    if (WInputManager::GetInputActionState("Console", "MoveCaretStart") == WKeyState::Pressed)
      MoveCaret(-1000);
    if (WInputManager::GetInputActionState("Console", "MoveCaretEnd") == WKeyState::Pressed)
      MoveCaret(1000);
    if (WInputManager::GetInputActionState("Console", "DeleteCharacter") == WKeyState::Pressed)
      DeleteNextCharacter();
    if (WInputManager::GetInputActionState("Console", "ScrollUp") == WKeyState::Pressed)
      Scroll(10);
    if (WInputManager::GetInputActionState("Console", "ScrollDown") == WKeyState::Pressed)
      Scroll(-10);
    if (WInputManager::GetInputActionState("Console", "HistoryUp") == WKeyState::Pressed)
    {
      RetrieveInputHistory(1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }
    if (WInputManager::GetInputActionState("Console", "HistoryDown") == WKeyState::Pressed)
    {
      RetrieveInputHistory(-1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }

    const WString sChars = WInputManager::RetrieveLastCharacters();

    for (auto it = sChars.GetIteratorFront(); it.IsValid(); ++it)
    {
      AddInputCharacter(it.GetCharacter());
    }
  }
  else
  {
    // Only the first character of whatever was typed this frame binds a quick-execute command; peek
    // rather than consume, since the console being closed means nobody else is waiting for this input.
    const WString sChars = WInputManager::RetrieveLastCharacters(false);
    const WUInt32 uiChar = sChars.GetIteratorFront().GetCharacter();

    char szCmd[16] = "";
    char* szIterator = szCmd;
    WUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (WInputManager::GetInputActionState("Console", "RepeatLast") == WKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ExecuteCommand(GetInputHistory()[0]);
  }

  if (WInputManager::GetInputActionState("Console", "RepeatSecondLast") == WKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ExecuteCommand(GetInputHistory()[1]);
  }
}

void WQuakeConsole::RenderConsole(bool bIsOpen)
{
  if (!bIsOpen)
    return;

  const WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  WViewHandle hView = pView->GetHandle();
  const float fViewWidth = pView->GetViewport().width;
  const float fViewHeight = pView->GetViewport().height;
  const float fGlyphWidth = WDebugRenderer::GetTextGlyphWidth();
  const float fLineHeight = WDebugRenderer::GetTextLineHeight();

  const float fConsoleHeight = fViewHeight * 0.4f;
  const float fBorderWidth = 2.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - (2.0f * fBorderWidth) - fLineHeight;

  const WInt32 iTextHeight = (WInt32)fLineHeight;
  const WInt32 iTextLeft = (WInt32)(fBorderWidth);

  // Draw console background
  {
    WColor backgroundColor(0.3f, 0.3f, 0.3f, 0.7f);
    WDebugRenderer::Draw2DRectangle(hView, WRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);
    WColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    WDebugRenderer::Draw2DRectangle(hView, WRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    WDebugRenderer::Draw2DRectangle(hView, WRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fLineHeight), 0.0f, foregroundColor);
  }

  // Draw console text
  {
    W_LOCK(GetMutex());
    auto& consoleStrings = GetConsoleStrings();
    WUInt32 uiNumConsoleLines = (WUInt32)(WMath::Ceil(fConsoleTextAreaHeight / fLineHeight));
    WInt32 uiFirstLine = GetScrollPosition() + uiNumConsoleLines - 1;
    WInt32 iFirstLinePos = (WInt32)(fBorderWidth);

    WInt32 uiSkippedLines = WMath::Max(uiFirstLine - (WInt32)consoleStrings.GetCount() + 1, 0);
    for (WUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      WDebugRenderer::Draw2DText(hView, consoleString.m_sText.GetData(), WVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
    }

    // Draw input line
    WDebugRenderer::Draw2DText(hView, GetInputLine(), WVec2I32(iTextLeft, (WInt32)(fConsoleTextAreaHeight + fBorderWidth + (fLineHeight * 0.5f))), WColor::White, 16, WDebugTextHAlign::Default, WDebugTextVAlign::Center);

    // Draw caret
    if (WMath::Fraction(WClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      const float fCaretPosition = (float)GetCaretPosition();
      const float fCaretX = fBorderWidth + (fCaretPosition + 0.5f) * fGlyphWidth;
      const float fCaretY = fConsoleTextAreaHeight + fBorderWidth + 1.0f;
      WColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      WDebugRenderer::Draw2DRectangle(hView, WRectFloat(fCaretX, fCaretY, 2.0f, fLineHeight - 2.0f), 0.0f, caretColor);
    }
  }
}

void WQuakeConsole::HandleInput(bool bIsOpen)
{
  DoDefaultInputHandling(bIsOpen);
}
