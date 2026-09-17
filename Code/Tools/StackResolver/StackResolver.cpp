#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/StackTracer.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <DbgHelp.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

struct Module
{
  WString m_sFilePath;
  WUInt64 m_uiBaseAddress;
  WUInt32 m_uiSize;
};

struct Stackframe
{
  WUInt32 m_uiModuleIndex = 0xFFFFFFFF;
  WUInt32 m_uiLineNumber = 0;
  WString m_sFilename;
  WString m_sSymbol;
};

WCommandLineOptionString opt_ModuleList("_app", "-ModuleList", "List of modules as a string in this format:\n\n\
File1Path?File1BaseAddressHEX?File1Size|File2Path?File2BaseAddressHEX?File2Size|...\n\n\
For example:\n\
  $[A]/app.exe?7FF7E5540000?106496|$[S]/System32/KERNELBASE.dll?7FFE2B780000?2920448\n\n\
  $[A] represents the application directory and will be adjusted as necessary.\n\
  $[S] represents the system root directory and will be adjusted as necessary.",
  "");
WCommandLineOptionString opt_Callstack("_app", "-Callstack", "Callstack in this format:\n\n7FFE2DD6CE74|7FFE2B7AAA86|7FFE034C22D1", "");

WCommandLineOptionEnum opt_OutputFormat("_app", "-Format", "How to output the resolved callstack.", "Text=0|JSON=1", 0);

WCommandLineOptionPath opt_OutputFile("_app", "-File", "The target file where to write the output to.\nIf left empty, the output is printed to the console.", "");

class WStackResolver : public WApplication
{
public:
  using SUPER = WApplication;

  WStackResolver()
    : WApplication("WStackResolver")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Run() override;

  WResult LoadModules();
  WResult ParseModules();
  WResult ParseCallstack();

  void ResolveStackFrames();
  void FormatAsText(WStringBuilder& ref_sOutput);
  void FormatAsJSON(WStringBuilder& ref_sOutput);

  HANDLE m_hProcess;
  WDynamicArray<Module> m_Modules;
  WDynamicArray<WUInt64> m_Callstack;
  WDynamicArray<Stackframe> m_Stackframes;
  WStringBuilder m_SystemRootDir;
  WStringBuilder m_ApplicationDir;
};

void WStackResolver::AfterCoreSystemsStartup()
{
  WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

  m_Modules.Reserve(128);
  m_Callstack.Reserve(128);
  m_Stackframes.Reserve(128);
}

void WStackResolver::BeforeCoreSystemsShutdown()
{
  WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
}

WResult WStackResolver::ParseModules()
{
  const WStringBuilder sModules = opt_ModuleList.GetOptionValue(WCommandLineOption::LogMode::Never);

  WDynamicArray<WStringView> parts;
  sModules.Split(false, parts, "|");

  for (WStringView sModView : parts)
  {
    WStringBuilder sMod = sModView;
    WDynamicArray<WStringView> parts2;
    sMod.Split(false, parts2, "?");

    WUInt64 base;
    if (WConversionUtils::ConvertHexStringToUInt64(parts2[1], base).Failed())
    {
      WLog::Error("Failed to convert HEX string '{}' to UINT64", parts2[1]);
      return W_FAILURE;
    }

    WStringBuilder sSize = parts2[2];
    WUInt32 size;
    if (WConversionUtils::StringToUInt(sSize, size).Failed())
    {
      WLog::Error("Failed to convert string '{}' to UINT32", sSize);
      return W_FAILURE;
    }

    WStringBuilder sModuleName = parts2[0];
    sModuleName.ReplaceFirst_NoCase("$[S]", m_SystemRootDir);
    sModuleName.ReplaceFirst_NoCase("$[A]", m_ApplicationDir);
    sModuleName.MakeCleanPath();

    auto& mod = m_Modules.ExpandAndGetRef();
    mod.m_sFilePath = sModuleName;
    mod.m_uiBaseAddress = base;
    mod.m_uiSize = size;
  }

  return W_SUCCESS;
}

WResult WStackResolver::ParseCallstack()
{
  WStringBuilder sCallstack = opt_Callstack.GetOptionValue(WCommandLineOption::LogMode::Never);

  WDynamicArray<WStringView> parts;
  sCallstack.Split(false, parts, "|");
  for (WStringView sModView : parts)
  {
    WUInt64 base;
    if (WConversionUtils::ConvertHexStringToUInt64(sModView, base).Failed())
    {
      WLog::Error("Failed to convert HEX string '{}' to UINT64", sModView);
      return W_FAILURE;
    }

    m_Callstack.PushBack(base);
  }

  return W_SUCCESS;
}

WResult WStackResolver::LoadModules()
{
  if (SymInitialize(m_hProcess, nullptr, FALSE) != TRUE) // TODO specify PDB search path as second parameter?
  {
    WLog::Error("SymInitialize failed");
    return W_FAILURE;
  }

  for (const auto& curModule : m_Modules)
  {
    if (SymLoadModuleExW(m_hProcess, nullptr, WStringWChar(curModule.m_sFilePath), nullptr, curModule.m_uiBaseAddress, curModule.m_uiSize, nullptr, 0) == 0)
    {
      WLog::Warning("Couldn't load module '{}'", curModule.m_sFilePath);
    }
    else
    {
      WLog::Success("Loaded module '{}'", curModule.m_sFilePath);
    }
  }

  return W_SUCCESS;
}

void WStackResolver::ResolveStackFrames()
{
  WStringBuilder tmp;

  char buffer[1024];
  for (WUInt32 i = 0; i < m_Callstack.GetCount(); i++)
  {
    DWORD64 symbolAddress = m_Callstack[i];

    _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
    WMemoryUtils::ZeroFill(&symbolInfo, 1);
    symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
    symbolInfo.MaxNameLen = (W_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

    DWORD64 displacement = 0;
    BOOL result = SymFromAddrW(m_hProcess, symbolAddress, &displacement, &symbolInfo);
    if (!result)
    {
      wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
    }

    IMAGEHLP_LINEW64 lineInfo;
    DWORD displacement2 = static_cast<DWORD>(displacement);
    WMemoryUtils::ZeroFill(&lineInfo, 1);
    lineInfo.SizeOfStruct = sizeof(lineInfo);
    SymGetLineFromAddrW64(m_hProcess, symbolAddress, &displacement2, &lineInfo);

    auto& frame = m_Stackframes.ExpandAndGetRef();

    for (WUInt32 modIndex = 0; modIndex < m_Modules.GetCount(); modIndex++)
    {
      if (m_Modules[modIndex].m_uiBaseAddress == (WUInt64)symbolInfo.ModBase)
      {
        frame.m_uiModuleIndex = modIndex;
        break;
      }
    }

    frame.m_uiLineNumber = (WUInt32)lineInfo.LineNumber;
    frame.m_sSymbol = WStringUtf8(symbolInfo.Name).GetView();

    tmp = WStringUtf8(lineInfo.FileName).GetView();
    tmp.MakeCleanPath();
    frame.m_sFilename = tmp;
  }
}

void WStackResolver::FormatAsText(WStringBuilder& ref_sOutput)
{
  WLog::Info("Formatting callstack as text.");

  for (const auto& frame : m_Stackframes)
  {
    WStringView sModuleName = "<unknown module>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    WStringView sFileName = "<unknown file>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    WStringView sSymbol = "<unknown symbol>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    ref_sOutput.AppendFormat("[][{}] {}({}): '{}'\n", sModuleName, sFileName, frame.m_uiLineNumber, sSymbol);
  }
}

void WStackResolver::FormatAsJSON(WStringBuilder& ref_sOutput)
{
  WLog::Info("Formatting callstack as JSON.");

  WContiguousMemoryStreamStorage storage;
  WMemoryStreamWriter writer(&storage);

  WStandardJSONWriter json;
  json.SetOutputStream(&writer);
  json.SetWhitespaceMode(WJSONWriter::WhitespaceMode::LessIndentation);

  json.BeginObject();
  json.BeginArray("Stackframes");

  for (const auto& frame : m_Stackframes)
  {
    WStringView sModuleName = "<unknown>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    WStringView sFileName = "<unknown>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    WStringView sSymbol = "<unknown>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    json.BeginObject();
    json.AddVariableString("Module", sModuleName);
    json.AddVariableString("File", sFileName);
    json.AddVariableUInt32("Line", frame.m_uiLineNumber);
    json.AddVariableString("Symbol", sSymbol);
    json.EndObject();
  }

  json.EndArray();
  json.EndObject();

  WStringView text((const char*)storage.GetData(), storage.GetStorageSize32());

  ref_sOutput.Append(text);
}

void WStackResolver::Run()
{
  if (WCommandLineOption::LogAvailableOptions(WCommandLineOption::LogAvailableModes::IfHelpRequested, "_app"))
  {
    QuitApplication();
    return;
  }

  WString sMissingOpt;
  if (WCommandLineOption::RequireOptions("-ModuleList;-Callstack", &sMissingOpt).Failed())
  {
    WLog::Error("Command line option '{}' was not specified.", sMissingOpt);

    WCommandLineOption::LogAvailableOptions(WCommandLineOption::LogAvailableModes::Always, "_app");
    QuitApplication();
    return;
  }

  m_hProcess = GetCurrentProcess();

  m_ApplicationDir = WOSFile::GetApplicationDirectory();
  m_ApplicationDir.MakeCleanPath();
  m_ApplicationDir.Trim("", "/");

  m_SystemRootDir = WEnvironmentVariableUtils::GetValueString("SystemRoot");
  m_SystemRootDir.MakeCleanPath();
  m_SystemRootDir.Trim("", "/");

  if (ParseModules().Failed())
  {
    QuitApplication();
    return;
  }

  if (ParseCallstack().Failed())
  {
    QuitApplication();
    return;
  }

  if (LoadModules().Failed())
  {
    QuitApplication();
    return;
  }

  ResolveStackFrames();

  WStringBuilder output;

  if (opt_OutputFormat.GetOptionValue(WCommandLineOption::LogMode::Never) == 0)
  {
    FormatAsText(output);
  }
  else if (opt_OutputFormat.GetOptionValue(WCommandLineOption::LogMode::Never) == 1)
  {
    FormatAsJSON(output);
  }

  if (opt_OutputFile.IsOptionSpecified())
  {
    WLog::Info("Writing output to '{}'.", opt_OutputFile.GetOptionValue(WCommandLineOption::LogMode::Never));

    WOSFile file;
    if (file.Open(opt_OutputFile.GetOptionValue(WCommandLineOption::LogMode::Never), WFileOpenMode::Write).Failed())
    {
      WLog::Error("Could not open file for writing: '{}'", opt_OutputFile.GetOptionValue(WCommandLineOption::LogMode::Never));
      QuitApplication();
      return;
    }

    file.Write(output.GetData(), output.GetElementCount()).IgnoreResult();
  }
  else
  {
    WLog::Info("Writing output to console.");

    W_LOG_BLOCK("Resolved callstack");

    WDynamicArray<WStringView> lines;
    output.Split(true, lines, "\n");

    for (auto l : lines)
    {
      WLog::Info("{}", l);
    }
  }

  QuitApplication();
}

W_APPLICATION_ENTRY_POINT(WStackResolver);
