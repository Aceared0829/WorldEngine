#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WIDE, 1)
  W_ENUM_CONSTANT(WIDE::DefaultProgram),
#if W_ENABLED(W_PLATFORM_WINDOWS)
  W_ENUM_CONSTANT(WIDE::VisualStudio),
#endif
  W_ENUM_CONSTANT(WIDE::VisualStudioCode),
  W_ENUM_CONSTANT(WIDE::Rider),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_ENUM(WCompiler, 1)
  W_ENUM_CONSTANT(WCompiler::Clang),
#if W_ENABLED(W_PLATFORM_LINUX)
  W_ENUM_CONSTANT(WCompiler::Gcc),
#elif W_ENABLED(W_PLATFORM_WINDOWS)
  W_ENUM_CONSTANT(WCompiler::Vs2022),
  W_ENUM_CONSTANT(WCompiler::Vs2026),
#endif
W_END_STATIC_REFLECTED_ENUM;

#if W_ENABLED(W_PLATFORM_LINUX)
#define CPP_COMPILER_DEFAULT "g++"
#define C_COMPILER_DEFAULT "gcc"
#elif W_ENABLED(W_PLATFORM_WINDOWS)
#define CPP_COMPILER_DEFAULT ""
#define C_COMPILER_DEFAULT ""
#else
#error Platform not implemented
#endif

W_BEGIN_STATIC_REFLECTED_TYPE(WCompilerPreferences, WNoBase, 1, WRTTIDefaultAllocator<WCompilerPreferences>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Compiler", WCompiler, m_Compiler)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("CustomCompiler", m_bCustomCompiler)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("CppCompiler", m_sCppCompiler)->AddAttributes(new WDefaultValueAttribute(CPP_COMPILER_DEFAULT)),
    W_MEMBER_PROPERTY("CCompiler", m_sCCompiler)->AddAttributes(new WDefaultValueAttribute(C_COMPILER_DEFAULT)),
    W_MEMBER_PROPERTY("RcCompiler", m_sRcCompiler),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_TYPE(WCodeEditorPreferences, WNoBase, 1, WRTTIDefaultAllocator<WCodeEditorPreferences>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("CodeEditorPath", m_sEditorPath)->AddAttributes(new WExternalFileBrowserAttribute("Select Editor", "*.exe"_wsv)),
    W_MEMBER_PROPERTY("CodeEditorArgs", m_sEditorArgs)->AddAttributes(new WDefaultValueAttribute("{file} {line}")),
    W_MEMBER_PROPERTY("IsVisualStudio", m_bIsVisualStudio)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCppProject, 1, WRTTIDefaultAllocator<WCppProject>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("CppIDE", WIDE, m_Ide),
    W_MEMBER_PROPERTY("CompilerPreferences", m_CompilerPreferences),
    W_MEMBER_PROPERTY("CodeEditorPreferences", m_CodeEditorPreferences),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEvent<const WCppSettings&> WCppProject::s_ChangeEvents;

WDynamicArray<WCppProject::MachineSpecificCompilerPaths> WCppProject::s_MachineSpecificCompilers;

namespace
{
  static constexpr WUInt32 minGccVersion = 10;
  static constexpr WUInt32 maxGccVersion = 20;
  static constexpr WUInt32 minClangVersion = 10;
  static constexpr WUInt32 maxClangVersion = 20;

  WResult TestCompilerExecutable(WStringView sName, WString* out_pVersion = nullptr)
  {
    WStringBuilder sStdout;
    WProcessOptions po;
    po.AddArgument("--version");
    po.m_sProcess = sName;
    po.m_onStdOut = [&sStdout](WStringView out)
    { sStdout.Append(out); };

    if (WProcess::Execute(po).Failed())
      return W_FAILURE;

    WTempHybridArray<WStringView, 8> lines;
    sStdout.Split(false, lines, "\r", "\n");
    if (lines.IsEmpty())
      return W_FAILURE;

    WTempHybridArray<WStringView, 4> splitResult;
    lines[0].Split(false, splitResult, " ");

    if (splitResult.IsEmpty())
      return W_FAILURE;

    WStringView version;
    do
    {
      version = splitResult.PeekBack();
      if (version.FindSubString("."))
      {
        break;
      }
      splitResult.PopBack();
    } while (!splitResult.IsEmpty());
    splitResult.Clear();
    version.Split(false, splitResult, ".");
    if (splitResult.GetCount() < 3)
    {
      return W_FAILURE;
    }

    if (out_pVersion)
    {
      *out_pVersion = version;
    }

    return W_SUCCESS;
  }

  void AddCompilerVersions(WDynamicArray<WCppProject::MachineSpecificCompilerPaths>& inout_compilers, WCompiler::Enum compiler, WStringView sRequiredMajorVersion)
  {
    WStringView compilerBaseName;
    WStringView compilerBaseNameCpp;
    switch (compiler)
    {
      case WCompiler::Clang:
        compilerBaseName = "clang";
        compilerBaseNameCpp = "clang++";
        break;
#if W_ENABLED(W_PLATFORM_LINUX)
      case WCompiler::Gcc:
        compilerBaseName = "gcc";
        compilerBaseNameCpp = "g++";
        break;
#endif
      default:
        W_ASSERT_NOT_IMPLEMENTED
    }


    WString compilerVersion;
    WStringBuilder requiredVersion = sRequiredMajorVersion;
    requiredVersion.Append('.');
    WStringBuilder fmt;
    if (TestCompilerExecutable(compilerBaseName, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerBaseNameCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (system default = {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerBaseName, compilerBaseNameCpp, false});
    }

    WStringBuilder compilerExecutable;
    WStringBuilder compilerExecutableCpp;
    compilerExecutable.SetFormat("{}-{}", compilerBaseName, sRequiredMajorVersion);
    compilerExecutableCpp.SetFormat("{}-{}", compilerBaseNameCpp, sRequiredMajorVersion);
    if (TestCompilerExecutable(compilerExecutable, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerExecutableCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.SetFormat("{} (version {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerExecutable, compilerExecutableCpp, false});
    }
  }
} // namespace

WString WCppProject::GetTargetSourceDir(WStringView sProjectDirectory /*= {}*/)
{
  WStringBuilder sTargetDir = sProjectDirectory;

  if (sTargetDir.IsEmpty())
  {
    sTargetDir = WToolsProject::GetSingleton()->GetProjectDirectory();
  }

  sTargetDir.AppendPath("CppSource");
  return sTargetDir;
}

WString WCppProject::GetGeneratorFolderName(const WCppSettings& cfg)
{
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)
    case WCompiler::Vs2022:
      return "Vs2022x64";
    case WCompiler::Vs2026:
      return "Vs2026x64";
#endif
    case WCompiler::Clang:
      return "Clangx64";
#if W_ENABLED(W_PLATFORM_LINUX)
    case WCompiler::Gcc:
      return "Gccx64";
#endif
  }
  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

WString WCppProject::GetCMakeGeneratorName(const WCppSettings& cfg)
{

#if W_ENABLED(W_PLATFORM_WINDOWS)
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
    case WCompiler::Vs2022:
      return "Visual Studio 17 2022";
    case WCompiler::Vs2026:
      return "Visual Studio 18 2026";
    case WCompiler::Clang:
      return "Ninja";
  }
#elif W_ENABLED(W_PLATFORM_LINUX)
  return "Ninja";
#else
#  error Platform not implemented
#endif
  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

WString WCppProject::GetPluginSourceDir(const WCppSettings& cfg, WStringView sProjectDirectory /*= {}*/)
{
  WStringBuilder sDir = GetTargetSourceDir(sProjectDirectory);
  sDir.AppendPath(cfg.m_sPluginName);
  sDir.Append("Plugin");
  return sDir;
}

WString WCppProject::GetBuildDir(const WCppSettings& cfg)
{
  WStringBuilder sBuildDir;
  sBuildDir.SetFormat("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

WString WCppProject::GetSolutionPath(const WCppSettings& cfg)
{
  WStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);

#if W_ENABLED(W_PLATFORM_WINDOWS)
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2022)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".sln");
    return sSolutionFile;
  }
  if (preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2026)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".slnx");
    return sSolutionFile;
  }
#endif

  sSolutionFile.AppendPath("build.ninja");
  return sSolutionFile;
}

WStatus WCppProject::OpenSolution(const WCppSettings& cfg)
{
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  switch (preferences->m_Ide.GetValue())
  {
    case WIDE::DefaultProgram:
    {
      if (WQtUiServices::OpenFileInDefaultProgram(WCppProject::GetSolutionPath(cfg)).Failed())
      {
        return WStatus("Failed to open solution with default program.\n\nGo to 'Tools > Preferences > C++ Projects' to select another option.");
      }

      return WStatus(W_SUCCESS);
    }

#if W_ENABLED(W_PLATFORM_WINDOWS)
    case WIDE::VisualStudio:
    {
      if (WQtUiServices::OpenInVisualStudio(WCppProject::GetSolutionPath(cfg)).Failed())
      {
        return WStatus("Failed to open solution with Visual Studio.\n\nGo to 'Tools > Preferences > C++ Projects' to select another option.");
      }

      return WStatus(W_SUCCESS);
    }
#endif

    case WIDE::Rider:
    {
#if W_ENABLED(W_PLATFORM_WINDOWS)
      auto solutionPath = WCppProject::GetSolutionPath(cfg);
#else
      auto solutionPath = WCppProject::GetTargetSourceDir();
#endif

      if (WQtUiServices::OpenInRider(solutionPath).Failed())
      {
        return WStatus("Failed to open solution with Rider.\n\nGo to 'Tools > Preferences > C++ Projects' to select another option.");
      }

      return WStatus(W_SUCCESS);
    }

    case WIDE::VisualStudioCode:
    {
      auto solutionPath = WCppProject::GetTargetSourceDir();
      QStringList args;
      args.push_back(QString::fromUtf8(solutionPath.GetData(), solutionPath.GetElementCount()));
      if (WStatus status = WQtUiServices::OpenInVsCode(args); status.Failed())
      {
        return WStatus(WFmt("Failed to open solution with Visual Studio Code: {}\n\nGo to 'Tools > Preferences > C++ Projects' to select another option.", status.GetMessageString()));
      }

      return WStatus(W_SUCCESS);
    }
  }

  return WStatus("Failed to open solution: Unknown error");
}

WStatus WCppProject::OpenInCodeEditor(const WStringView& sFileName, WInt32 iLineNumber)
{
  if (!WOSFile::ExistsFile(sFileName))
  {
    return WStatus("Failed finding filename");
  }

  WStringBuilder sLineNumber;
  WConversionUtils::ToString(iLineNumber, sLineNumber);

  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  // Visual Studio does not expose a CLI command to open a file/line in all use-cases directly
  // therefore run a custom .vbs script which controls VS and performs the needed actions for us. This avoids pulling COM interfacing into the project.
  if (preferences->m_CodeEditorPreferences.m_bIsVisualStudio)
  {
    WStringBuilder dir;
    if (WFileSystem::ResolveSpecialDirectory(">sdk/Utilities/Scripts/open-in-msvs.vbs", dir).Failed())
    {
      return WStatus("Failed resolving path to \">sdk/Utilities/Scripts/open-in-msvs.vbs\"");
    }

    if (!WOSFile::ExistsFile(dir))
    {
      return WStatus(WFmt("File does not exist '{0}'", dir));
    }

    QStringList args;
    args.append("/B");
    args.append(QString::fromUtf8(dir.GetData()));
    args.append(QString::fromUtf8(sFileName.GetStartPointer(), sFileName.GetElementCount()));
    args.append(QString::fromUtf8(sLineNumber.GetData()));

    QProcess proc;
    if (proc.startDetached("cscript", args) == false)
    {
      return WStatus("Failed to launch code editor");
    }

    return WStatus(W_SUCCESS);
  }


  WStringBuilder sFormatString = preferences->m_CodeEditorPreferences.m_sEditorArgs;
  if (sFormatString.IsEmpty())
  {
    return WStatus("Code editor is not configured");
  }

  sFormatString.ReplaceAll("{line}", sLineNumber);
  sFormatString.ReplaceAll("{file}", sFileName);

  const QStringList args = QProcess::splitCommand(QString::fromUtf8(sFormatString.GetData()));
  const QString sProgramPath = QString::fromUtf8(preferences->m_CodeEditorPreferences.m_sEditorPath.GetData());

  QProcess proc;
  if (proc.startDetached(sProgramPath, args) == false)
  {
    return WStatus("Failed to launch code editor");
  }
  return WStatus(W_SUCCESS);
}

WStringView WCppProject::CompilerToString(WCompiler::Enum compiler)
{
  switch (compiler)
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)
    case WCompiler::Vs2022:
      return "Vs2022";
    case WCompiler::Vs2026:
      return "Vs2026";
#endif
    case WCompiler::Clang:
      return "Clang";
#if W_ENABLED(W_PLATFORM_LINUX)
    case WCompiler::Gcc:
      return "Gcc";
#endif
    default:
      break;
  }
  W_ASSERT_NOT_IMPLEMENTED;
  return "<not implemented>";
}

WCompiler::Enum WCppProject::GetSdkCompiler()
{
#if W_ENABLED(W_COMPILER_CLANG)
  return WCompiler::Clang;
#elif W_ENABLED(W_COMPILER_GCC)
  return WCompiler::Gcc;
#elif W_ENABLED(W_COMPILER_MSVC)
#  if _MSC_VER >= 1950
  return WCompiler::Vs2026;
#  else
  return WCompiler::Vs2022;
#  endif
#else
#  error Unknown compiler
#endif
}

WString WCppProject::GetSdkCompilerMajorVersion()
{
#if W_ENABLED(W_COMPILER_MSVC)
  WStringBuilder fmt;
  fmt.SetFormat("{}.{}", _MSC_VER / 100, _MSC_VER % 100);
  return fmt;
#elif W_ENABLED(W_COMPILER_CLANG)
  return W_PP_STRINGIFY(__clang_major__);
#elif W_ENABLED(W_COMPILER_GCC)
  return W_PP_STRINGIFY(__GNUC__);
#else
#  error Unsupported compiler
#endif
}

WStatus WCppProject::TestCompiler()
{
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler != GetSdkCompiler())
  {
    return WStatus(WFmt("The currently configured compiler is incompatible with this SDK. The SDK was built with '{}' but the currently configured compiler is '{}'.",
      CompilerToString(GetSdkCompiler()),
      CompilerToString(preferences->m_CompilerPreferences.m_Compiler)));
  }

#if W_ENABLED(W_PLATFORM_WINDOWS)
  // As CMake is selecting the compiler it is hard to do a version check, for now just assume they are compatible.
  if (GetSdkCompiler() == WCompiler::Vs2022 || GetSdkCompiler() == WCompiler::Vs2026)
  {
    return WStatus(W_SUCCESS);
  }

  if (GetSdkCompiler() == WCompiler::Clang)
  {
    if (!WOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler))
    {
      return WStatus(WFmt("The selected RC compiler '{}' does not exist on disk.", preferences->m_CompilerPreferences.m_sRcCompiler));
    }
  }
#endif

  WString cCompilerVersion, cppCompilerVersion;
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCCompiler, &cCompilerVersion).Failed())
  {
    return WStatus("The selected C Compiler doesn't work or doesn't exist.");
  }
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCppCompiler, &cppCompilerVersion).Failed())
  {
    return WStatus("The selected C++ Compiler doesn't work or doesn't exist.");
  }

  WStringBuilder sdkCompilerMajorVersion = GetSdkCompilerMajorVersion();
  sdkCompilerMajorVersion.Append('.');
  if (!cCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return WStatus(WFmt("The selected C Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cCompilerVersion));
  }
  if (!cppCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return WStatus(WFmt("The selected C++ Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cppCompilerVersion));
  }

  return WStatus(W_SUCCESS);
}

const char* WCppProject::GetCMakePath()
{
#if W_ENABLED(W_PLATFORM_WINDOWS)
  return "cmake/bin/cmake";
#elif W_ENABLED(W_PLATFORM_LINUX)
  return "cmake";
#else
#  error Platform not implemented
#endif
}

WResult WCppProject::CheckCMakeCache(const WCppSettings& cfg)
{
  WStringBuilder sCacheFile;
  sCacheFile = GetBuildDir(cfg);
  sCacheFile.AppendPath("CMakeCache.txt");

  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sCacheFile));

  WStringBuilder content;
  content.ReadAll(file);

  const WStringView sSearchFor = "CMAKE_CONFIGURATION_TYPES:STRING="_wsv;

  const char* pConfig = content.FindSubString(sSearchFor);
  if (pConfig == nullptr)
    return W_FAILURE;

  pConfig += sSearchFor.GetElementCount();

  const char* pEndConfig = content.FindSubString("\n", pConfig);
  if (pEndConfig == nullptr)
    return W_FAILURE;

  WStringBuilder sUsedCfg;
  sUsedCfg.SetSubString_FromTo(pConfig, pEndConfig);
  sUsedCfg.Trim("\t\n\r ");

  if (sUsedCfg != BUILDSYSTEM_BUILDTYPE)
    return W_FAILURE;

  return W_SUCCESS;
}

WCppProject::ModifyResult WCppProject::CheckCMakeUserPresets(const WCppSettings& cfg, bool bWriteResult)
{
  WStringBuilder configureJsonPath = WCppProject::GetPluginSourceDir(cfg).GetFileDirectory();
  configureJsonPath.AppendPath("CMakeUserPresets.json");

  if (WOSFile::ExistsFile(configureJsonPath))
  {
    WFileReader fileReader;
    if (fileReader.Open(configureJsonPath).Failed())
    {
      WLog::Error("Failed to open '{}' for reading", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    WJSONReader reader;
    if (reader.Parse(fileReader).Failed())
    {
      WLog::Error("Failed to parse JSON of '{}'", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    fileReader.Close();

    if (reader.GetTopLevelElementType() != WJSONReader::ElementType::Dictionary)
    {
      WLog::Error("Top level element of '{}' is expected to be a dictionary. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    WVariantDictionary json = reader.GetTopLevelObject();
    auto modifyResult = ModifyCMakeUserPresetsJson(cfg, json);
    if (modifyResult == ModifyResult::FAILURE)
    {
      WLog::Error("Failed to modify '{}' in place. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    if (bWriteResult && modifyResult == ModifyResult::MODIFIED)
    {
      WStandardJSONWriter jsonWriter;
      WDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(WVariant(json));
      if (fileWriter.Close().Failed())
      {
        WLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }

    return modifyResult;
  }
  else
  {
    if (bWriteResult)
    {
      WStandardJSONWriter jsonWriter;
      WDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(WVariant(CreateEmptyCMakeUserPresetsJson(cfg)));
      if (fileWriter.Close().Failed())
      {
        WLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }
  }

  return ModifyResult::MODIFIED;
}

bool WCppProject::ExistsSolution(const WCppSettings& cfg)
{
  return WOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool WCppProject::ExistsProjectCMakeListsTxt()
{
  if (!WToolsProject::IsProjectOpen())
    return false;

  WStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return WOSFile::ExistsFile(sPath);
}

bool WCppProject::ShouldOverwriteExisting(WStringView sSrc, WStringView sDst)
{
  const WStringView sFilename = sDst.GetFileNameAndExtension();

  // only check certain files
  // they use a "#W-version" tag to identify when a file got modified in such a way
  // that existing files should be overwritten
  if (sFilename != "CMakeLists.txt" && sFilename != ".clang-format" && sFilename != ".editorconfig" && sFilename != ".gitattributes" && sFilename != ".gitignore")
  {
    return false;
  }

  WOSFile srcFile;
  if (srcFile.Open(sSrc, WFileOpenMode::Read).Failed())
    return false;

  WOSFile dstFile;
  if (dstFile.Open(sDst, WFileOpenMode::Read).Failed())
    return true;

  WDataBuffer sc, dc;
  srcFile.ReadAll(sc);
  dstFile.ReadAll(dc);

  if (sc == dc)
    return false;

  WStringView srcContent = WStringView((const char*)sc.GetData(), sc.GetCount());
  WStringView dstContent = WStringView((const char*)dc.GetData(), dc.GetCount());

  WStringView sVersionSrc, sVersionDst;

  if (const char* vSrc = srcContent.FindSubString("#W-version"))
  {
    const char* srcEnd = srcContent.FindSubString("\n", vSrc);

    if (srcEnd == nullptr)
    {
      srcEnd = srcContent.GetEndPointer();
    }

    sVersionSrc = WStringView(vSrc + 8, srcEnd);
    sVersionSrc.Trim();
  }

  if (const char* vDst = dstContent.FindSubString("#W-version"))
  {
    const char* dstEnd = dstContent.FindSubString("\n", vDst);

    if (dstEnd == nullptr)
    {
      dstEnd = dstContent.GetEndPointer();
    }

    sVersionDst = WStringView(vDst + 8, dstEnd);
    sVersionDst.Trim();
  }

  // don't overwrite the destination file, if it is different, but at the same version
  if (sVersionSrc == sVersionDst)
    return false;

  return true;
}

WResult WCppProject::PopulateWithDefaultSources(const WCppSettings& cfg, WUInt32* pNumFilesCopied /*= nullptr*/)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  W_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const WString sProjectName = cfg.m_sPluginName;

  WStringBuilder sProjectNameUpper = cfg.m_sPluginName;
  sProjectNameUpper.ToUpper();

  const WStringBuilder sTargetDir = WToolsProject::GetSingleton()->GetProjectDirectory();

  WStringBuilder sSourceDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  WDynamicArray<WFileStats> items;
  WOSFile::GatherAllItemsInFolder(items, sSourceDir, WFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    WString m_sSource;
    WString m_sDestination;
  };

  WTempHybridArray<FileToCopy, 32> filesCopied;

  // gather files
  {
    bool bCheckOverwrite = false;

    for (const auto& item : items)
    {
      WStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("CppProject", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (WOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        if (!bCheckOverwrite)
        {
          // first time we see an existing file, clear the list of files to copy and enter different mode
          // from now on, we only add files to the copy list, that should get overwritten
          filesCopied.Clear();
          bCheckOverwrite = true;
        }
      }

      if (bCheckOverwrite)
      {
        if (!ShouldOverwriteExisting(srcPath, dstPath))
        {
          // in this mode, don't copy files unless they should be overwritten
          // mostly to update existing CMakeLists.txt files with newer versions
          continue;
        }
      }

      auto& ftc = filesCopied.ExpandAndGetRef();
      ftc.m_sSource = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  if (pNumFilesCopied)
  {
    *pNumFilesCopied = filesCopied.GetCount();
  }

  // Copy files
  {
    for (const auto& ftc : filesCopied)
    {
      if (WOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        WLog::Error("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return W_FAILURE;
      }
    }
  }

  // Modify sources
  {
    for (const auto& filePath : filesCopied)
    {
      WStringBuilder content;

      {
        WFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          WLog::Error("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return W_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("CppProject", sProjectName);
      content.ReplaceAll("CPPPROJECT", sProjectNameUpper);

      {
        WFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          WLog::Error("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return W_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  W_SUCCEED_OR_RETURN(UpdateEnginePluginDependencies());

  s_ChangeEvents.Broadcast(cfg);
  return W_SUCCESS;
}

WResult WCppProject::CleanBuildDir(const WCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  W_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const WString sBuildDir = GetBuildDir(cfg);

  if (!WOSFile::ExistsDirectory(sBuildDir))
    return W_SUCCESS;

  return WOSFile::DeleteFolder(sBuildDir);
}

WStatus WCppProject::CheckBuildPathLength(const WCppSettings& cfg)
{
#if W_ENABLED(W_PLATFORM_WINDOWS)
  // Longest relative path observed below the build directory, e.g.
  // "CMakeFiles/<version>/VCTargetsPath.tlog/ParallelCustomBuild.read.1.tlog". Targets nested deeper than
  // that exist, so this is rounded up rather than taken as an exact bound. MSBuild writes these paths
  // through .NET APIs that enforce MAX_PATH, which is why WOSFile's long path support does not help.
  constexpr WUInt32 uiReservedForGeneratedFiles = 100;
  constexpr WUInt32 uiMaxPath = 260;

  const WStringBuilder sBuildDir = GetBuildDir(cfg);
  const WUInt32 uiBuildDirLength = sBuildDir.GetElementCount();

  if (uiBuildDirLength + uiReservedForGeneratedFiles <= uiMaxPath)
    return WStatus(W_SUCCESS);

  return WStatus(WFmt("The C++ build directory is too long for this system:\n\n{}\n\nIt uses {} of the {} characters Windows allows, leaving too little room for the files the build system creates below it. Move the project to a shorter directory.", sBuildDir, uiBuildDirLength, uiMaxPath));
#else
  W_IGNORE_UNUSED(cfg);
  return WStatus(W_SUCCESS);
#endif
}

WResult WCppProject::RunCMake(const WCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  W_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  if (!ExistsProjectCMakeListsTxt())
  {
    WLog::Error("No CMakeLists.txt exists in target source directory '{}'", GetTargetSourceDir());
    return W_FAILURE;
  }

  if (auto pathLength = CheckBuildPathLength(cfg); pathLength.Failed())
  {
    pathLength.LogFailure();
    return W_FAILURE;
  }

  W_SUCCEED_OR_RETURN(UpdateEnginePluginDependencies());

  if (auto compilerWorking = TestCompiler(); compilerWorking.Failed())
  {
    compilerWorking.LogFailure();
    return W_FAILURE;
  }

  if (CheckCMakeUserPresets(cfg, true) == ModifyResult::FAILURE)
  {
    return W_FAILURE;
  }

  WStringBuilder tmp;

  QStringList args;
  args << "--preset";
  args << "WorldEngine";

  WLogSystemToBuffer log;


  const WString sTargetSourceDir = WCppProject::GetTargetSourceDir();

  WStatus res = WQtEditorApp::GetSingleton()->ExecuteTool(GetCMakePath(), args, 120, &log, WLogMsgType::InfoMsg, sTargetSourceDir);

  if (res.Failed())
  {
    WLog::Error("CMake generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.GetMessageString());
    return W_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    WLog::Error("CMake did not generate the expected output. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return W_FAILURE;
  }

  WLog::Success("CMake generation successful.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return W_SUCCESS;
}

WResult WCppProject::RunCMakeIfNecessary(const WCppSettings& cfg)
{
  if (!WCppProject::ExistsProjectCMakeListsTxt())
    return W_SUCCESS;

  auto userPresetResult = CheckCMakeUserPresets(cfg, false);
  if (userPresetResult == ModifyResult::FAILURE)
  {
    return W_FAILURE;
  }

  if (WCppProject::ExistsSolution(cfg) && WCppProject::CheckCMakeCache(cfg).Succeeded() && userPresetResult == ModifyResult::NOT_MODIFIED)
    return W_SUCCESS;

  return WCppProject::RunCMake(cfg);
}

WResult WCppProject::CompileSolution(const WCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  W_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  W_LOG_BLOCK("Compile C++ Plugin");

  WTempHybridArray<WString, 32> errors;
  WInt32 iReturnCode = 0;
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  if (WSystemInformation::IsDebuggerAttached())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  WProcessOptions po;

  WString cmakePath = WQtEditorApp::GetSingleton()->FindToolApplication(WCppProject::GetCMakePath());

  po.m_sProcess = cmakePath;
  po.AddArgument("--build");
  po.AddArgument(GetBuildDir(cfg));
#if W_ENABLED(W_PLATFORM_WINDOWS)
  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  if (preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2022 || preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2026)
  {
    po.AddArgument("--config");
    po.AddArgument(BUILDSYSTEM_BUILDTYPE);
  }
#endif
  po.m_sWorkingDirectory = GetBuildDir(cfg);
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](WStringView sText)
  {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };
  po.m_onStdError = [&](WStringView sText)
  {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };

  WStringBuilder sCMakeBuildCmd;
  po.BuildCommandLineString(sCMakeBuildCmd);
  WLog::Dev("Running {} {}", cmakePath, sCMakeBuildCmd);
  if (WProcess::Execute(po, &iReturnCode).Failed())
  {
    WLog::Error("Failed to start CMake.");
    return W_FAILURE;
  }

  if (iReturnCode == 0)
  {
    WLog::Success("Compiled C++ code.");
    return W_SUCCESS;
  }

  WLog::Error("CMake --build failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    WLog::Error(err);
  }

  return W_FAILURE;
}

WResult WCppProject::BuildCodeIfNecessary(const WCppSettings& cfg)
{
  if (!WCppProject::ExistsProjectCMakeListsTxt())
    return W_SUCCESS;

  // Also re-runs CMake when the generated CMakeUserPresets.json no longer matches the current
  // configuration - a missing solution and a stale cache are not the only reasons to regenerate, e.g.
  // the SDK's output directories change when the editor is started from a different build.
  W_SUCCEED_OR_RETURN(WCppProject::RunCMakeIfNecessary(cfg));

  return CompileSolution(cfg);
}

WVariantDictionary WCppProject::CreateEmptyCMakeUserPresetsJson(const WCppSettings& cfg)
{
  WVariantDictionary json;
  json.Insert("version", 3);

  {
    WVariantDictionary cmakeMinimumRequired;
    cmakeMinimumRequired.Insert("major", 3);
    cmakeMinimumRequired.Insert("minor", 21);
    cmakeMinimumRequired.Insert("patch", 0);

    json.Insert("cmakeMinimumRequired", std::move(cmakeMinimumRequired));
  }

  {
    WVariantArray configurePresets;
    WVariantDictionary WEnginePreset;
    WEnginePreset.Insert("name", "WorldEngine");
    WEnginePreset.Insert("displayName", "Build the WorldEngine Plugin");

    {
      WVariantDictionary cacheVariables;
      WEnginePreset.Insert("cacheVariables", std::move(cacheVariables));
    }

    configurePresets.PushBack(std::move(WEnginePreset));
    json.Insert("configurePresets", std::move(configurePresets));
  }

  {
    WVariantArray buildPresets;
    {
      WVariantDictionary WEngineBuildPreset;
      WEngineBuildPreset.Insert("name", "WorldEngine");
      WEngineBuildPreset.Insert("configurePreset", "WorldEngine");
      buildPresets.PushBack(std::move(WEngineBuildPreset));
    }
    json.Insert("buildPresets", std::move(buildPresets));
  }

  W_VERIFY(ModifyCMakeUserPresetsJson(cfg, json) == ModifyResult::MODIFIED, "Freshly created user presets file should always be modified");

  return json;
}

void WCppProject::UpdatePluginConfig(const WCppSettings& cfg)
{
  const WStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  WPluginBundleSet& bundles = WQtEditorApp::GetSingleton()->GetPluginBundles();

  WStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  WPluginBundle& plugin = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy = true;
  plugin.m_bAllowEnableReload = true;
  plugin.m_bSelected = true;
  plugin.m_bMissing = true;
  plugin.m_LastModificationTime = WTimestamp::MakeInvalid();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  WQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}

WResult WCppProject::UpdateEnginePluginDependencies()
{
  // Only update if project has custom C++ code
  if (!ExistsProjectCMakeListsTxt())
    return W_SUCCESS;

  const WPluginBundleSet& bundles = WQtEditorApp::GetSingleton()->GetPluginBundles();

  // Collect selected engine plugin target names
  WDynamicArray<WString> selectedPlugins;
  for (auto it = bundles.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    const WPluginBundle& bundle = it.Value();

    // Filter: selected, not mandatory, not a project plugin, has CMake target name
    if (bundle.m_bSelected &&
        !bundle.m_bMandatory &&
        !bundle.m_ExclusiveFeatures.Contains("ProjectPlugin") &&
        !bundle.m_sCMakeTargetName.IsEmpty())
    {
      selectedPlugins.PushBack(bundle.m_sCMakeTargetName);
    }
  }

  // Sort plugins for consistent output
  selectedPlugins.Sort();

  // Build the CMake file content
  WStringBuilder content;
  content.AppendWithSeparator("# This file is auto-generated, do not modify.\n", "");
  content.Append("# The WEditor may modify this file to add build configuration options.\n");
  content.Append("\n");

  // Only add target_link_libraries if there are selected plugins
  if (!selectedPlugins.IsEmpty())
  {
    content.Append("\n");
    content.Append("# Link against selected engine plugins\n");
    content.Append("target_link_libraries(${PROJECT_NAME} PRIVATE\n");

    for (const WString& plugin : selectedPlugins)
    {
      content.Append("  ", plugin, "\n");
    }

    content.Append(")\n");
  }

  // Write to file
  WStringBuilder sFilePath = GetTargetSourceDir();
  sFilePath.AppendPath("Configs/CMakeEngineExtensions.txt");

  // Ensure directory exists
  WStringBuilder sDir = sFilePath.GetFileDirectory();
  if (WOSFile::CreateDirectoryStructure(sDir).Failed())
  {
    WLog::Error("Failed to create directory for CMakeEngineExtensions.txt: '{}'", sDir);
    return W_FAILURE;
  }

  // Write file
  WDeferredFileWriter fileWriter;
  fileWriter.SetOutput(sFilePath);

  if (fileWriter.WriteBytes(content.GetData(), content.GetElementCount()).Failed())
  {
    WLog::Error("Failed to write CMakeEngineExtensions.txt: '{}'", sFilePath);
    return W_FAILURE;
  }

  if (fileWriter.Close().Failed())
  {
    WLog::Error("Failed to close CMakeEngineExtensions.txt: '{}'", sFilePath);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WCppProject::EnsureCppPluginReady()
{
  if (!ExistsProjectCMakeListsTxt())
    return W_SUCCESS;

  WCppSettings cppSettings;
  if (cppSettings.Load().Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning(WFmt("Failed to load the C++ plugin settings."));
    return W_FAILURE;
  }

  if (WCppProject::PopulateWithDefaultSources(cppSettings).Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning(WFmt("Failed to update the default source files of the plugin. See log for details."));
    return W_FAILURE;
  }

  if (WCppProject::BuildCodeIfNecessary(cppSettings).Failed())
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning(WFmt("Failed to build the C++ code. See log for details."));
    return W_FAILURE;
  }

  WQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  return W_SUCCESS;
}

bool WCppProject::IsBuildRequired()
{
  if (!ExistsProjectCMakeListsTxt())
    return false;

  WCppSettings cfg;
  if (cfg.Load().Failed())
    return false;

  if (!WCppProject::ExistsSolution(cfg))
    return true;

  if (WCppProject::CheckCMakeCache(cfg).Failed())
    return true;

  WStringBuilder sPath = WOSFile::GetApplicationDirectory();
  sPath.AppendPath(cfg.m_sPluginName);

  sPath.Append("Plugin");

#if W_ENABLED(W_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  if (!WOSFile::ExistsFile(sPath))
    return true;

  return false;
}

namespace
{
  template <typename T>
  T* Expect(WVariantDictionary& inout_json, WStringView sName)
  {
    WVariant* var = nullptr;
    if (inout_json.TryGetValue(sName, var) && var->IsA<T>())
    {
      return &var->GetWritable<T>();
    }
    return nullptr;
  }

  void Modify(WVariantDictionary& inout_json, WStringView sName, WStringView sValue, WCppProject::ModifyResult& inout_modified)
  {
    WVariant* currentValue = nullptr;
    if (inout_json.TryGetValue(sName, currentValue) && currentValue->IsA<WString>() && currentValue->Get<WString>() == sValue)
      return;

    inout_json[sName] = sValue;
    inout_modified = WCppProject::ModifyResult::MODIFIED;
  }

  void Remove(WVariantDictionary& inout_json, WStringView sName, WCppProject::ModifyResult& inout_modified)
  {
    if (inout_json.Remove(sName))
    {
      inout_modified = WCppProject::ModifyResult::MODIFIED;
    }
  }

  /// Determines the output directories of the SDK build that this application belongs to.
  ///
  /// A plugin has to be built into the same directories, otherwise the editor does not find it. They
  /// cannot be derived from the SDK root: an SDK built with a custom output directory (a separate
  /// workspace, CI) puts its binaries elsewhere, and then '<sdk>/Output/Bin' is a directory of some
  /// other build, or does not exist.
  ///
  /// The DLL directory is therefore taken from where this application actually runs - the parent of
  /// the '<platform><compiler><config>' folder that the executable sits in. It must NOT be taken from
  /// 'WExportInfo.cmake': the paths in that file are those of the machine that built the SDK, and
  /// serve as the patterns that W_include_WExport() replaces. In a release package they point at the
  /// build server and nothing exists there.
  ///
  /// The file is still needed for the LIB directory, which cannot be observed at runtime. Only the
  /// position of that directory *relative to* the DLL directory is used from it, so that a build with
  /// an unusual layout keeps its layout.
  ///
  /// Returns W_FAILURE when the file is missing or unreadable, in which case the caller should not
  /// specify the directories at all and let the plugin's CMakeLists.txt fall back to its default.
  WResult ReadSdkOutputDirectories(WStringBuilder& out_sDllDir, WStringBuilder& out_sLibDir)
  {
    WStringBuilder sDllDir = WOSFile::GetApplicationDirectory();
    sDllDir.MakeCleanPath();
    sDllDir.PathParentDirectory(); // strip the '<platform><compiler><config>' folder
    sDllDir.Trim(nullptr, "/");

    WStringBuilder sFile = sDllDir;
    sFile.AppendPath("WExportInfo.cmake");
    sFile.MakeCleanPath();

    WOSFile file;
    W_SUCCEED_OR_RETURN(file.Open(sFile, WFileOpenMode::Read));

    WStringBuilder sContent;
    {
      WDataBuffer content;
      file.ReadAll(content);
      sContent = WStringView((const char*)content.GetData(), content.GetCount());
    }

    auto ReadValue = [&](WStringView sVariable, WStringBuilder& out_sValue) -> WResult
    {
      WStringBuilder sPrefix("set(", sVariable, " ");

      const char* szStart = sContent.FindSubString(sPrefix);
      if (szStart == nullptr)
        return W_FAILURE;

      szStart += sPrefix.GetElementCount();

      const char* szEnd = sContent.FindSubString(")", szStart);
      if (szEnd == nullptr)
        return W_FAILURE;

      out_sValue.SetSubString_FromTo(szStart, szEnd);
      out_sValue.Trim(" \t\r\n\"");
      return out_sValue.IsEmpty() ? W_FAILURE : W_SUCCESS;
    };

    WStringBuilder sBuildDllDir, sBuildLibDir;
    W_SUCCEED_OR_RETURN(ReadValue("EXPINP_OUTPUT_DIRECTORY_DLL", sBuildDllDir));
    W_SUCCEED_OR_RETURN(ReadValue("EXPINP_OUTPUT_DIRECTORY_LIB", sBuildLibDir));

    sBuildDllDir.MakeCleanPath();
    sBuildLibDir.MakeCleanPath();

    // typically '../Lib'
    W_SUCCEED_OR_RETURN(sBuildLibDir.MakeRelativeTo(sBuildDllDir));

    out_sDllDir = sDllDir;

    out_sLibDir = sDllDir;
    out_sLibDir.AppendPath(sBuildLibDir);
    out_sLibDir.MakeCleanPath();

    return W_SUCCESS;
  }

} // namespace

WCppProject::ModifyResult WCppProject::ModifyCMakeUserPresetsJson(const WCppSettings& cfg, WVariantDictionary& inout_json)
{
  auto result = ModifyResult::NOT_MODIFIED;
  auto configurePresets = Expect<WVariantArray>(inout_json, "configurePresets");
  if (!configurePresets)
    return ModifyResult::FAILURE;

  const WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  for (auto& preset : *configurePresets)
  {
    if (!preset.IsA<WVariantDictionary>())
      continue;

    auto& presetDict = preset.GetWritable<WVariantDictionary>();

    auto name = Expect<WString>(presetDict, "name");
    if (!name || *name != "WorldEngine")
    {
      continue;
    }

    auto cacheVariables = Expect<WVariantDictionary>(presetDict, "cacheVariables");
    if (!cacheVariables)
      return ModifyResult::FAILURE;

    Modify(*cacheVariables, "W_SDK_DIR", WFileSystem::GetSdkRootDirectory(), result);

    // Without these the plugin is built into '<sdk>/Output/Bin', which is not where this application
    // was loaded from when the SDK was built into a custom output directory. The plugin would compile
    // and the editor would still not find it.
    {
      WStringBuilder sDllDir, sLibDir;
      if (ReadSdkOutputDirectories(sDllDir, sLibDir).Succeeded())
      {
        Modify(*cacheVariables, "W_OUTPUT_DIRECTORY_DLL", sDllDir, result);
        Modify(*cacheVariables, "W_OUTPUT_DIRECTORY_LIB", sLibDir, result);
      }
    }

    Modify(*cacheVariables, "W_BUILDTYPE_ONLY", BUILDSYSTEM_BUILDTYPE, result);
    Modify(*cacheVariables, "CMAKE_BUILD_TYPE", BUILDSYSTEM_BUILDTYPE, result);

    bool needsCompilerPaths = true;
#if W_ENABLED(W_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2022 || preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2026)
    {
      needsCompilerPaths = false;
    }
#endif

    if (needsCompilerPaths)
    {
      Modify(*cacheVariables, "CMAKE_C_COMPILER", preferences->m_CompilerPreferences.m_sCCompiler, result);
      Modify(*cacheVariables, "CMAKE_CXX_COMPILER", preferences->m_CompilerPreferences.m_sCppCompiler, result);
#if W_ENABLED(W_PLATFORM_WINDOWS)
      Modify(*cacheVariables, "CMAKE_RC_COMPILER", preferences->m_CompilerPreferences.m_sRcCompiler, result);
      Modify(*cacheVariables, "CMAKE_RC_COMPILER_INIT", "rc", result);
#endif
    }
    else
    {
      cacheVariables->Remove("CMAKE_C_COMPILER");
      cacheVariables->Remove("CMAKE_CXX_COMPILER");
#if W_ENABLED(W_PLATFORM_WINDOWS)
      cacheVariables->Remove("CMAKE_RC_COMPILER");
      cacheVariables->Remove("CMAKE_RC_COMPILER_INIT");
#endif
    }

    Modify(presetDict, "generator", GetCMakeGeneratorName(cfg), result);
    Modify(presetDict, "binaryDir", GetBuildDir(cfg), result);
#if W_ENABLED(W_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2022 || preferences->m_CompilerPreferences.m_Compiler == WCompiler::Vs2026)
    {
      Modify(presetDict, "architecture", "x64", result);
    }
    else
#endif
    {
      Remove(presetDict, "architecture", result);
    }
  }

  return result;
}

WCppProject::WCppProject()
  : WPreferences(WPreferences::Domain::Application, "C++ Projects")
{
  m_CompilerPreferences.m_Compiler = GetSdkCompiler();
}
void WCppProject::LoadPreferences()
{
  W_PROFILE_SCOPE("Preferences");
  auto preferences = WPreferences::QueryPreferences<WCppProject>();

  WCompiler::Enum sdkCompiler = GetSdkCompiler();

#if W_ENABLED(W_PLATFORM_WINDOWS)
  if (sdkCompiler == WCompiler::Vs2022)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2022 (system default)", WCompiler::Vs2022, "", "", false});
  }
  if (sdkCompiler == WCompiler::Vs2026)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2026 (system default)", WCompiler::Vs2026, "", "", false});
  }

#  if W_ENABLED(W_COMPILER_CLANG)
  // if the rcCompiler path is empty or points to a non existant file, try to autodetect it
  if ((preferences->m_CompilerPreferences.m_sRcCompiler.IsEmpty() || !WOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler)))
  {
    WStringBuilder rcPath;
    HKEY hInstalledRoots = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hInstalledRoots) == ERROR_SUCCESS)
    {
      W_SCOPE_EXIT(RegCloseKey(hInstalledRoots));
      DWORD pathLengthInBytes = 0;
      WDynamicArray<wchar_t> path;
      if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, nullptr, &pathLengthInBytes) == ERROR_SUCCESS)
      {
        path.SetCount(pathLengthInBytes / sizeof(wchar_t));
        if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, path.GetData(), &pathLengthInBytes) == ERROR_SUCCESS)
        {
          WStringBuilder windowsSdkBinPath;
          windowsSdkBinPath = WStringWChar(path.GetData());
          windowsSdkBinPath.MakeCleanPath();
          windowsSdkBinPath.AppendPath("bin");

          WDynamicArray<WFileStats> folders;
          WOSFile::GatherAllItemsInFolder(folders, windowsSdkBinPath, WFileSystemIteratorFlags::ReportFolders);

          folders.Sort([](const WFileStats& a, const WFileStats& b)
            { return a.m_sName > b.m_sName; });

          for (const WFileStats& folder : folders)
          {
            if (!folder.m_sName.StartsWith("10."))
            {
              continue;
            }
            rcPath = windowsSdkBinPath;
            rcPath.AppendPath(folder.m_sName);
            rcPath.AppendPath("x64/rc.exe");
            if (WOSFile::ExistsFile(rcPath))
            {
              break;
            }
            rcPath.Clear();
          }
        }
      }
    }
    if (!rcPath.IsEmpty())
    {
      preferences->m_CompilerPreferences.m_sRcCompiler = rcPath;
    }
  }

  WString clangVersion;

  wchar_t* pProgramFiles = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &pProgramFiles)))
  {
    WStringBuilder clangDefaultPath;
    clangDefaultPath = WStringWChar(pProgramFiles);
    CoTaskMemFree(pProgramFiles);
    pProgramFiles = nullptr;

    clangDefaultPath.AppendPath("LLVM/bin/clang.exe");
    clangDefaultPath.MakeCleanPath();
    WStringBuilder clangCppDefaultPath = clangDefaultPath;
    clangCppDefaultPath.ReplaceLast(".exe", "++.exe");

    WStringView clangMajorSdkVersion = W_PP_STRINGIFY(__clang_major__) ".";
    if (TestCompilerExecutable(clangDefaultPath, &clangVersion).Succeeded() && TestCompilerExecutable(clangCppDefaultPath).Succeeded() && clangVersion.StartsWith(clangMajorSdkVersion))
    {
      WStringBuilder clangNiceName;
      clangNiceName.SetFormat("Clang (system default = {})", clangVersion);
      s_MachineSpecificCompilers.PushBack({clangNiceName, WCompiler::Clang, clangDefaultPath, clangCppDefaultPath, false});
    }
  }
#  endif
#endif


#if W_ENABLED(W_PLATFORM_LINUX)
  AddCompilerVersions(s_MachineSpecificCompilers, WCppProject::GetSdkCompiler(), WCppProject::GetSdkCompilerMajorVersion());
#endif

#if W_ENABLED(W_COMPILER_CLANG)
  s_MachineSpecificCompilers.PushBack({"Clang (Custom)", WCompiler::Clang, "", "", true});
#endif

#if W_ENABLED(W_PLATFORM_LINUX) && W_ENABLED(W_COMPILER_GCC)
  s_MachineSpecificCompilers.PushBack({"Gcc (Custom)", WCompiler::Gcc, "", "", true});
#endif

  if (preferences->m_CompilerPreferences.m_Compiler != sdkCompiler)
  {
    WStringBuilder incompatibleCompilerName = u8"⚠ ";
    incompatibleCompilerName.SetFormat(u8"⚠ {} (incompatible)", WCppProject::CompilerToString(preferences->m_CompilerPreferences.m_Compiler));
    s_MachineSpecificCompilers.PushBack(
      {incompatibleCompilerName,
        preferences->m_CompilerPreferences.m_Compiler,
        preferences->m_CompilerPreferences.m_sCCompiler,
        preferences->m_CompilerPreferences.m_sCppCompiler,
        preferences->m_CompilerPreferences.m_bCustomCompiler});
  }
}

WResult WCppProject::ForceSdkCompatibleCompiler()
{
  WCppProject* preferences = WPreferences::QueryPreferences<WCppProject>();

  WCompiler::Enum sdkCompiler = GetSdkCompiler();
  for (auto& compiler : s_MachineSpecificCompilers)
  {
    if (!compiler.m_bIsCustom && compiler.m_Compiler == sdkCompiler)
    {
      preferences->m_CompilerPreferences.m_Compiler = sdkCompiler;
      preferences->m_CompilerPreferences.m_sCCompiler = compiler.m_sCCompiler;
      preferences->m_CompilerPreferences.m_sCppCompiler = compiler.m_sCppCompiler;
      preferences->m_CompilerPreferences.m_bCustomCompiler = compiler.m_bIsCustom;

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

WCppProject::~WCppProject() = default;
