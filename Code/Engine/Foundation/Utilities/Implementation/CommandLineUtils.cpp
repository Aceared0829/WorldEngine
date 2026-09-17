#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <shellapi.h>
#endif

static WCommandLineUtils g_pCmdLineInstance;

WCommandLineUtils* WCommandLineUtils::GetGlobalInstance()
{
  return &g_pCmdLineInstance;
}

void WCommandLineUtils::SplitCommandLineString(WStringView sCommandString, bool bAddExecutableDir, WDynamicArray<WString>& out_args, WDynamicArray<const char*>& out_argsV)
{
  // Add application dir as first argument as customary on other platforms.
  if (bAddExecutableDir)
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)
    wchar_t moduleFilename[256];
    GetModuleFileNameW(nullptr, moduleFilename, 256);
    out_args.PushBack(WStringUtf8(moduleFilename).GetData());
#else
    W_ASSERT_NOT_IMPLEMENTED;
#endif
  }

  // Simple args splitting. Not as powerful as Win32's CommandLineToArgvW.
  // Supports double-quoted tokens that may contain spaces. Quotes are stripped from the result.
  bool bInQuotes = false;
  WStringBuilder current;

  for (auto it = sCommandString.GetIteratorFront(); it.IsValid(); ++it)
  {
    const WUInt32 uiChar = it.GetCharacter();

    if (uiChar == '\"')
    {
      bInQuotes = !bInQuotes;
    }
    else if (uiChar == ' ' && !bInQuotes)
    {
      if (!current.IsEmpty())
      {
        out_args.PushBack(current);
        current.Clear();
      }
    }
    else
    {
      current.Append(uiChar);
    }
  }

  if (!current.IsEmpty())
  {
    out_args.PushBack(current);
  }

  out_argsV.Reserve(out_args.GetCount());
  for (WString& str : out_args)
    out_argsV.PushBack(str.GetData());
}

void WCommandLineUtils::SetCommandLine(WUInt32 uiArgc, const char** pArgv, ArgMode mode /*= UseArgcArgv*/)
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  if (mode == ArgMode::PreferOsArgs)
  {
    SetCommandLine();
    return;
  }
#else
  W_IGNORE_UNUSED(mode);
#endif

  m_Commands.Clear();
  m_Commands.Reserve(uiArgc);

  for (WUInt32 i = 0; i < uiArgc; ++i)
    m_Commands.PushBack(pArgv[i]);
}

void WCommandLineUtils::SetCommandLine(WArrayPtr<WString> commands)
{
  m_Commands = commands;
}

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)

void WCommandLineUtils::SetCommandLine()
{
  int argc = 0;

  LPWSTR* argvw = CommandLineToArgvW(::GetCommandLineW(), &argc);

  W_ASSERT_RELEASE(argvw != nullptr, "CommandLineToArgvW failed");

  WArrayPtr<WStringUtf8> ArgvUtf8 = W_DEFAULT_NEW_ARRAY(WStringUtf8, argc);
  WArrayPtr<const char*> argv = W_DEFAULT_NEW_ARRAY(const char*, argc);

  for (WInt32 i = 0; i < argc; ++i)
  {
    ArgvUtf8[i] = argvw[i];
    argv[i] = ArgvUtf8[i].GetData();
  }

  SetCommandLine(argc, argv.GetPtr(), ArgMode::UseArgcArgv);


  W_DEFAULT_DELETE_ARRAY(ArgvUtf8);
  W_DEFAULT_DELETE_ARRAY(argv);
  LocalFree(argvw);
}

#endif

const WDynamicArray<WString>& WCommandLineUtils::GetCommandLineArray() const
{
  return m_Commands;
}

WString WCommandLineUtils::GetCommandLineString() const
{
  WStringBuilder commandLine;
  for (const WString& command : m_Commands)
  {
    if (commandLine.IsEmpty())
    {
      commandLine.Append(command.GetView());
    }
    else
    {
      commandLine.Append(" ", command);
    }
  }
  return commandLine;
}

WUInt32 WCommandLineUtils::GetParameterCount() const
{
  return m_Commands.GetCount();
}

const WString& WCommandLineUtils::GetParameter(WUInt32 uiParam) const
{
  return m_Commands[uiParam];
}

WInt32 WCommandLineUtils::GetOptionIndex(WStringView sOption, bool bCaseSensitive) const
{
  W_ASSERT_DEV(sOption.StartsWith("-"), "All command line option names must start with a hyphen (e.g. -file)");

  for (WUInt32 i = 0; i < m_Commands.GetCount(); ++i)
  {
    if ((bCaseSensitive && m_Commands[i].IsEqual(sOption)) || (!bCaseSensitive && m_Commands[i].IsEqual_NoCase(sOption)))
      return i;
  }

  return -1;
}

bool WCommandLineUtils::HasOption(WStringView sOption, bool bCaseSensitive /*= false*/) const
{
  return GetOptionIndex(sOption, bCaseSensitive) >= 0;
}

WUInt32 WCommandLineUtils::GetStringOptionArguments(WStringView sOption, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return 0;

  WUInt32 uiParamCount = 0;

  for (WUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> no parameters
      break;

    ++uiParamCount;
  }

  return uiParamCount;
}

WStringView WCommandLineUtils::GetStringOption(WStringView sOption, WUInt32 uiArgument, WStringView sDefault, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return sDefault;

  WUInt32 uiParamCount = 0;

  for (WUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> not enough parameters
      return sDefault;

    // found the right one, return it
    if (uiParamCount == uiArgument)
    {
      // We trim " as this is automatically done on Windows when parsing command line arguments and this will make it behave the same on Linux.
      WStringView sData = m_Commands[uiParam].GetView();
      sData.Trim("\"");
      return sData;
    }
    ++uiParamCount;
  }

  return sDefault;
}

const WString WCommandLineUtils::GetAbsolutePathOption(WStringView sOption, WUInt32 uiArgument /*= 0*/, WStringView sDefault /*= {} */, bool bCaseSensitive /*= false*/) const
{
  WStringView sPath = GetStringOption(sOption, uiArgument, sDefault, bCaseSensitive);
  if (sPath.IsEmpty())
    return sPath;

  return WOSFile::MakePathAbsoluteWithCWD(sPath);
}

bool WCommandLineUtils::GetBoolOption(WStringView sOption, bool bDefault, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return bDefault;

  const WUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount())    // last command, treat this as 'on'
    return true;

  if (m_Commands[uiIndex + 1].StartsWith("-")) // next command is the next option -> treat this as 'on' as well
    return true;

  // otherwise try to convert the next option to a boolean
  bool bRes = bDefault;
  WConversionUtils::StringToBool(m_Commands[uiIndex + 1].GetData(), bRes).IgnoreResult();

  return bRes;
}

WInt32 WCommandLineUtils::GetIntOption(WStringView sOption, WInt32 iDefault, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return iDefault;

  const WUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return iDefault;

  // try to convert the next option to a number
  WInt32 iRes = iDefault;
  WConversionUtils::StringToInt(m_Commands[uiIndex + 1].GetData(), iRes).IgnoreResult();

  return iRes;
}

WUInt32 WCommandLineUtils::GetUIntOption(WStringView sOption, WUInt32 uiDefault, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return uiDefault;

  const WUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return uiDefault;

  // try to convert the next option to a number
  WUInt32 uiRes = uiDefault;
  WConversionUtils::StringToUInt(m_Commands[uiIndex + 1].GetData(), uiRes).IgnoreResult();

  return uiRes;
}

double WCommandLineUtils::GetFloatOption(WStringView sOption, double fDefault, bool bCaseSensitive) const
{
  const WInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return fDefault;

  const WUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return fDefault;

  // try to convert the next option to a number
  double fRes = fDefault;
  WConversionUtils::StringToFloat(m_Commands[uiIndex + 1].GetData(), fRes).IgnoreResult();

  return fRes;
}

void WCommandLineUtils::InjectCustomArgument(WStringView sArgument)
{
  m_Commands.PushBack(sArgument);
}
