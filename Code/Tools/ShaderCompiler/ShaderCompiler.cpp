#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ShaderCompiler/ShaderCompiler.h>

WCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .WShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
  "");

WCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Absolute path to the folder of the project, for which shaders should be compiled.",
  "");

WCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform DX11_SM50\n\
  -platform VULKAN\n\
  -platform ALL",
  "DX11_SM50");

WCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

WCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE MSAA=TRUE\n\
\n\
If a permutation variable is not set to a fixed value, all shader permutations for that variable will generated and compiled.\n\
",
  "");

WShaderCompilerApplication::WShaderCompilerApplication()
  : WGameApplication("WShaderCompiler", nullptr)
{
}

WResult WShaderCompilerApplication::BeforeCoreSystemsStartup()
{
  {
    WStringBuilder cmdHelp;
    if (WCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, WCommandLineOption::LogAvailableModes::IfHelpRequested, "_ShaderCompiler"))
    {
      WLog::Print(cmdHelp);
      return W_FAILURE;
    }
  }

  WStartup::AddApplicationTag("tool");
  WStartup::AddApplicationTag("shadercompiler");

  // only print important messages
  WLog::SetDefaultLogLevel(WLogMsgType::InfoMsg);

  W_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  auto cmd = WCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = opt_Shader.GetOptionValue(WCommandLineOption::LogMode::Always);
  m_sAppProjectPath = opt_Project.GetOptionValue(WCommandLineOption::LogMode::Always);
  m_sPlatforms = opt_Platform.GetOptionValue(WCommandLineOption::LogMode::Always);
  opt_IgnoreErrors.GetOptionValue(WCommandLineOption::LogMode::Always);

  const WUInt32 pvs = cmd->GetStringOptionArguments("-perm");

  for (WUInt32 pv = 0; pv < pvs; ++pv)
  {
    WStringBuilder var = cmd->GetStringOption("-perm", pv);

    const char* szEqual = var.FindSubString("=");

    if (szEqual == nullptr)
    {
      WLog::Error("Permutation Variable declaration contains no equal sign: '{0}'", var);
      continue;
    }

    WStringBuilder val = szEqual + 1;
    var.SetSubString_FromTo(var.GetData(), szEqual);

    val.Trim(" \t");
    var.Trim(" \t");

    WLog::Dev("Fixed permutation variable: {0} = {1}", var, val);
    m_FixedPermVars[var].PushBack(val);
  }

  return W_SUCCESS;
}


void WShaderCompilerApplication::AfterCoreSystemsStartup()
{
  WSystemInformation info = WSystemInformation::Get();
  const WInt32 iCpuCores = info.GetCPUCoreCount();
  WTaskSystem::SetWorkerThreadCount(iCpuCores);

  ExecuteInitFunctions();

  WStartup::StartupHighLevelSystems();
}

WResult WShaderCompilerApplication::CompileShader(WStringView sShaderFile)
{
  W_PROFILE_SCOPE("WShaderCompilerApplication::CompileShader");
  W_LOG_BLOCK("Compiling Shader", sShaderFile);

  if (ExtractPermutationVarValues(sShaderFile).Failed())
    return W_FAILURE;


  const WUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  WLog::Info("Shader has {0} permutations", uiMaxPerms);

  bool bContinue = true;

  WTaskSystem::ParallelForIndexed(0, uiMaxPerms, [&](WUInt32 idx, WUInt32 num)
    {
      if (!bContinue)
        return;

      WTempHybridArray<WPermutationVar, 16> PermVars;

      WTokenizedFileCache fileCache;
      for (WUInt32 perm = idx; perm < num; ++perm)
      {
        W_PROFILE_SCOPE("CompilePermutation");
        W_LOG_BLOCK("Compiling Permutation");

        m_PermutationGenerator.GetPermutation(perm, PermVars);
        WShaderCompiler sc;
        if (sc.CompileShaderPermutationForPlatforms(sShaderFile, PermVars, WLog::GetThreadLocalLogSystem(), m_sPlatforms, &fileCache).Failed())
        {
          bContinue = false;
          return;
        }
      }
      //
    });

  if (!bContinue)
  {
    WLog::Error("Failed to compile shader '{0}'", sShaderFile);
    return W_FAILURE;
  }

  WLog::Success("Compiled Shader '{0}'", sShaderFile);
  return W_SUCCESS;
}

WResult WShaderCompilerApplication::ExtractPermutationVarValues(WStringView sShaderFile)
{
  W_PROFILE_SCOPE("WShaderCompilerApplication::ExtractPermutationVarValues");

  m_PermutationGenerator.Clear();

  WFileReader shaderFile;
  if (shaderFile.Open(sShaderFile).Failed())
  {
    WLog::Error("Could not open file '{0}'", sShaderFile);
    return W_FAILURE;
  }

  WString sContent;
  sContent.ReadAll(shaderFile);

  WShaderHelper::WTextSectionizer Sections;
  WShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  WTempHybridArray<WHashedString, 16> permVars;
  WTempHybridArray<WPermutationVar, 16> fixedPermVars;
  WUInt32 uiFirstLine = 0;
  WStringView sPermutations = Sections.GetSectionContent(WShaderHelper::WShaderSections::PERMUTATIONS, uiFirstLine);
  WShaderParser::ParsePermutationSection(sPermutations, permVars, fixedPermVars);

  {
    W_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      WLog::Dev(s.GetData());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permVars)
    {
      WTempHybridArray<WHashedString, 16> values;
      WShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermVars)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      WHashedString hsname, hsvalue;
      hsname.Assign(it.Key().GetData());
      m_PermutationGenerator.RemovePermutations(hsname);

      for (const auto& val : it.Value())
      {
        hsvalue.Assign(val.GetData());

        m_PermutationGenerator.AddPermutation(hsname, hsvalue);
      }
    }
  }

  return W_SUCCESS;
}

void WShaderCompilerApplication::PrintConfig()
{
  W_LOG_BLOCK("ShaderCompiler Config");

  WLog::Info("Project: '{0}'", m_sAppProjectPath);
  WLog::Info("Shader: '{0}'", m_sShaderFiles);
  WLog::Info("Platform: '{0}'", m_sPlatforms);
}

void WShaderCompilerApplication::Run()
{
  PrintConfig();

  W_LOG_BLOCK("Compile All Shaders");

  WDynamicArray<WString> shadersToCompile;

  WStringBuilder files = m_sShaderFiles;

  WDynamicArray<WStringView> allFiles;
  // If not shader files are provided, compile all shaders of the project, i.e. all data directories.
  if (m_sShaderFiles.IsEmpty())
  {
    WStringBuilder sPath, sPath2;
    for (WUInt32 dirIdx = 0; dirIdx < WFileSystem::GetNumDataDirectories(); ++dirIdx)
    {
      sPath = WFileSystem::GetDataDirectory(dirIdx)->GetDataDirectoryPath();

      if (sPath.IsEmpty())
        continue;

      if (WFileSystem::ResolveSpecialDirectory(sPath, sPath2).Failed())
        continue;

      files.AppendWithSeparator(";", sPath2);
    }
  }

  files.Split(false, allFiles, ";");

  WUInt32 uiErrors = 0;
  for (const WStringView& entry : allFiles)
  {
    WStringBuilder fileOrFolder;
    // Relative paths are always relative to the project
    if (WPathUtils::IsRelativePath(entry))
    {
      fileOrFolder = m_sAppProjectPath;
      fileOrFolder.AppendPath(entry);
    }
    else
    {
      fileOrFolder = entry;
    }

    WFileStats stats;
    if (WOSFile::GetFileStats(fileOrFolder, stats).Failed())
    {
      WLog::Error("Couldn't find path '{0}'", fileOrFolder);
      ++uiErrors;
      continue;
    }

    WStringBuilder relPath, absPath;
    if (stats.m_bIsDirectory)
    {
      WFileSystemIterator fsIt;
      WStringBuilder fullPath;
      for (fsIt.StartSearch(fileOrFolder, WFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
      {
        if (WPathUtils::HasExtension(fsIt.GetStats().m_sName, "WShader"))
        {
          fsIt.GetStats().GetFullPath(fullPath);
          if (WFileSystem::ResolvePath(fullPath, &absPath, &relPath).Succeeded())
          {
            shadersToCompile.PushBack(relPath);
          }
          else
          {
            WLog::Error("Couldn't resolve path '{0}'", fullPath);
            ++uiErrors;
          }
        }
      }
    }
    else if (WFileSystem::ResolvePath(fileOrFolder, &absPath, &relPath).Succeeded())
    {
      if (absPath.HasExtension("WShader"))
      {
        shadersToCompile.PushBack(relPath);
      }
      else
      {
        WLog::Error("File '{0}' is not a shader", absPath);
        ++uiErrors;
      }
    }
    else
    {
      WLog::Error("Couldn't resolve path '{0}'", fileOrFolder);
    }
  }

  for (const auto& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      ++uiErrors;
      if (!opt_IgnoreErrors.GetOptionValue(WCommandLineOption::LogMode::Never))
      {
        SetReturnCode(uiErrors);
        QuitApplication();
        return;
      }
    }
  }
  SetReturnCode(uiErrors);
  QuitApplication();
}

W_APPLICATION_ENTRY_POINT(WShaderCompilerApplication);
