#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WConsoleFunctionBase);

WColor WConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case WConsoleString::Type::Default:
      return WColor::White;

    case WConsoleString::Type::Error:
      return WColor(1.0f, 0.2f, 0.2f);

    case WConsoleString::Type::SeriousWarning:
      return WColor(1.0f, 0.4f, 0.1f);

    case WConsoleString::Type::Warning:
      return WColor(1.0f, 0.6f, 0.1f);

    case WConsoleString::Type::Note:
      return WColor(1, 200.0f / 255.0f, 0);

    case WConsoleString::Type::Success:
      return WColor(0.1f, 1.0f, 0.1f);

    case WConsoleString::Type::Executed:
      return WColor(1.0f, 0.5f, 0.0f);

    case WConsoleString::Type::VarName:
      return WColorGammaUB(255, 210, 0);

    case WConsoleString::Type::FuncName:
      return WColorGammaUB(100, 255, 100);

    case WConsoleString::Type::Dev:
      return WColor(0.6f, 0.6f, 0.6f);

    case WConsoleString::Type::Debug:
      return WColor(0.4f, 0.6f, 0.8f);

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return WColor::White;
}

WConsole::WConsole() = default;

WConsole::~WConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void WConsole::SetMainConsole(WConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

WConsole* WConsole::GetMainConsole()
{
  return s_pMainConsole;
}

WConsole* WConsole::s_pMainConsole = nullptr;

bool WConsole::AutoComplete(WStringBuilder& ref_sText)
{
  W_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    WCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (!s.m_sOutput.IsEmpty())
    {
      AddConsoleString("", WConsoleString::Type::Note);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void WConsole::ExecuteCommand(WStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  W_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    WCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void WConsole::AddConsoleString(WStringView sText, WConsoleString::Type type /*= WConsoleString::Type::Default*/)
{
  WConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  WConsoleEvent e;
  e.m_Type = WConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void WConsole::AddToInputHistory(WStringView sText)
{
  W_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (WInt32 i = 0; i < (WInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (WInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(WMath::Min<WUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (WUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void WConsole::RetrieveInputHistory(WInt32 iHistoryUp, WStringBuilder& ref_sResult)
{
  W_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = WMath::Clamp<WInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

WResult WConsole::SaveInputHistory(WStringView sFile)
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WStringBuilder str;

  for (const WString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    W_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return W_SUCCESS;
}

void WConsole::LoadInputHistory(WStringView sFile)
{
  WFileReader file;
  if (file.Open(sFile).Failed())
    return;

  WStringBuilder str;
  str.ReadAll(file);

  WTempHybridArray<WStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (WUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}
