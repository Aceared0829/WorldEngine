#include <Foundation/FoundationPCH.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES)

// Include platform specific implementation
#  include <Process_Platform.inl>

#  include <Foundation/Strings/Implementation/StringIterator.h>

WProcess::WProcess(WProcess&& rhs) = default;

void WProcessOptions::AddArgument(const WFormatString& arg)
{
  WStringBuilder tmp;
  m_Arguments.PushBack(arg.GetText(tmp));
}

void WProcessOptions::AddCommandLine(WStringView sCmdLine)
{
  WStringBuilder curArg;

  WStringView cmdView = sCmdLine;

  bool isInString = false;

  for (auto it = cmdView.GetIteratorFront(); it.IsValid(); ++it)
  {
    bool commit = false;
    bool commitEmpty = false;
    bool append = true;

    if (it.GetCharacter() == '\"')
    {
      append = false;

      if (isInString)
      {
        commitEmpty = true; // push-back even empty strings (they are there for a purpose)
      }
      else
      {
        commit = true; // only commit non-empty stuff that is not a string argument
      }

      isInString = !isInString;
    }
    else if (it.GetCharacter() == ' ')
    {
      if (!isInString)
      {
        commit = true;
        append = false;
      }
    }

    if (commitEmpty || (commit && !curArg.IsEmpty()))
    {
      m_Arguments.PushBack(curArg);
      curArg.Clear();
    }

    if (append)
    {
      curArg.Append(it.GetCharacter());
    }
  }

  if (!curArg.IsEmpty())
  {
    m_Arguments.PushBack(curArg);
    curArg.Clear();
  }
}

WInt32 WProcess::GetExitCode() const
{
  if (m_iExitCode == -0xFFFF)
  {
    // this may update m_iExitCode, if the state has switched to 'finished'
    GetState();
  }

  return m_iExitCode;
}

void WProcessOptions::BuildCommandLineString(WStringBuilder& ref_sCmd) const
{
  for (const auto& arg0 : m_Arguments)
  {
    WStringView arg = arg0;

    while (arg.StartsWith("\""))
      arg.ChopAwayFirstCharacterAscii();

    while (arg.EndsWith("\""))
      arg.Shrink(0, 1);

    // also wrap empty arguments in quotes, otherwise they would get lost
    if (arg.IsEmpty() || arg.FindSubString(" ") != nullptr || arg.FindSubString("\t") != nullptr || arg.FindSubString("\n") != nullptr)
    {
      ref_sCmd.Append(" \"");
      ref_sCmd.Append(arg);
      ref_sCmd.Append("\"");
    }
    else
    {
      ref_sCmd.Append(" ");
      ref_sCmd.Append(arg);
    }
  }

  ref_sCmd.Trim(" ");
}

void WProcess::BuildFullCommandLineString(const WProcessOptions& opt, WStringView sProcess, WStringBuilder& cmd) const
{
  // have to set the full path to the process as the very first argument
  cmd.Set("\"", sProcess, "\"");

  opt.BuildCommandLineString(cmd);
}
#endif
