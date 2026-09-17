#include <EditorTest/EditorTestPCH.h>

#include "GenerateCompile.h"
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <GuiFoundation/Action/ActionManager.h>

static WEditorTestGenerateCompilePacMan s_WEditorTestGenerateCompilePacMan;

const char* WEditorTestGenerateCompilePacMan::GetTestName() const
{
  return "Generate and Compile";
}

void WEditorTestGenerateCompilePacMan::SetupSubTests()
{
  AddSubTest("01 - Generate and Compile", SubTests::ST_GenerateAndCompile);
  AddSubTest("02 - EditorProcessor: Compile Only", SubTests::ST_EditorProcessorCompileOnly);
  AddSubTest("03 - EditorProcessor: Compile and Transform", SubTests::ST_EditorProcessorCompileAndTransform);
}

WResult WEditorTestGenerateCompilePacMan::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::OpenProject("Data/Samples/PacMan").Failed())
    return W_FAILURE;

  if (WCppProject::ForceSdkCompatibleCompiler().Failed())
  {
    WLog::Error("Failed to autodetect SDK compatible compiler for testing");
    return W_FAILURE;
  }
  WPreferences::SaveApplicationPreferences();

  return W_SUCCESS;
}

WResult WEditorTestGenerateCompilePacMan::DeInitializeTest()
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    WStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    WOSFile::DeleteFolder(buildOutput).IgnoreResult();
  }

  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WStatus WEditorTestGenerateCompile::PrepareCompile(WStringBuilder& dllPath)
{
  // Delete any existing build artifacts
  dllPath = WOSFile::GetApplicationDirectory();
  dllPath.AppendPath(m_sProjectName);
#if W_ENABLED(W_PLATFORM_WINDOWS)
  dllPath.Append("Plugin.dll");
#else
  dllPath.Append("Plugin.so");
#endif

  WOSFile::DeleteFile(dllPath).IgnoreResult();

  if (!W_TEST_BOOL(!WOSFile::ExistsFile(dllPath)))
  {
    return WStatus("Failed to delete existing build artifacts - DLL or bundle file still exists");
  }

  return WStatus(W_SUCCESS);
}

WStatus WEditorTestGenerateCompile::EditorProcessorCompileOnly()
{
  WStringBuilder dllPath;
  W_SUCCEED_OR_RETURN(PrepareCompile(dllPath));

  // Run EditorProcessor with -compile flag
  WDynamicArray<WString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sProjectPath);
  arguments.PushBack("-compile");
  arguments.PushBack("-outputDir");
  WStringBuilder userDataDir = WTestFramework::GetInstance()->GetAbsOutputPath();
  userDataDir.AppendPath(GetTestName());
  userDataDir.MakeCleanPath();
  arguments.PushBack(userDataDir);

  W_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // Verify that the DLL and bundle files have been created
  W_TEST_BOOL(WOSFile::ExistsFile(dllPath));

  return WStatus(W_SUCCESS);
}

WStatus WEditorTestGenerateCompile::EditorProcessorCompileAndTransform()
{
  // Delete AssetCache folder before the test
  WStringBuilder assetCachePath = m_sProjectPath;
  assetCachePath.AppendPath("AssetCache");

  if (WOSFile::ExistsDirectory(assetCachePath) && WOSFile::DeleteFolder(assetCachePath).Failed())
  {
    return WStatus(WFmt("Failed to delete asset cache folder: {}", assetCachePath));
  }

  WStringBuilder dllPath;
  W_SUCCEED_OR_RETURN(PrepareCompile(dllPath));

  // Run EditorProcessor with -compile and -transform flags
  WDynamicArray<WString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sProjectPath);
  arguments.PushBack("-compile");
  arguments.PushBack("-transform");
  arguments.PushBack("Default");
  arguments.PushBack("-outputDir");
  WStringBuilder userDataDir = WTestFramework::GetInstance()->GetAbsOutputPath();
  userDataDir.AppendPath(GetTestName());
  userDataDir.MakeCleanPath();
  arguments.PushBack(userDataDir);

  W_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // Verify that the AssetCache folder has been created
  W_TEST_BOOL(WOSFile::ExistsDirectory(assetCachePath));

  // Verify that Default.WAidlt exists in AssetCache
  WStringBuilder aidltPath = assetCachePath;
  aidltPath.AppendPath("Default.WAidlt");
  W_TEST_BOOL(WOSFile::ExistsFile(aidltPath));

  // Verify that the DLL and bundle files have been created
  W_TEST_BOOL(WOSFile::ExistsFile(dllPath));

  return WStatus(W_SUCCESS);
}

WStatus WEditorTestGenerateCompile::GenerateAndCompile()
{
  WStringBuilder dllPath;
  W_SUCCEED_OR_RETURN(PrepareCompile(dllPath));

  WCppSettings cpp;
  if (!W_TEST_RESULT(cpp.Load()))
    return WStatus(W_FAILURE);

  WString sBuildDir = WCppProject::GetBuildDir(cpp);
  if (WOSFile::ExistsDirectory(sBuildDir))
  {
    if (!W_TEST_RESULT(WOSFile::DeleteFolder(sBuildDir)))
      return WStatus(W_FAILURE);
  }

  if (!W_TEST_RESULT(WCppProject::RunCMake(cpp)))
    return WStatus(W_FAILURE);

  W_TEST_BOOL(WCppProject::ExistsProjectCMakeListsTxt());
  W_TEST_BOOL(WCppProject::ExistsSolution(cpp));

  W_TEST_RESULT(WCppProject::BuildCodeIfNecessary(cpp));
  ProcessEvents();
  return WStatus(W_SUCCESS);
}

WTestAppRun WEditorTestGenerateCompilePacMan::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ProcessEvents();
  switch (iIdentifier)
  {
    case SubTests::ST_GenerateAndCompile:
    {
      W_TEST_STATUS(GenerateAndCompile());
    }
    break;
    case SubTests::ST_EditorProcessorCompileOnly:
    {
      W_TEST_STATUS(EditorProcessorCompileOnly());
    }
    break;
    case SubTests::ST_EditorProcessorCompileAndTransform:
    {
      W_TEST_STATUS(EditorProcessorCompileAndTransform());
    }
    break;
  }
  ProcessEvents();
  return WTestAppRun::Quit;
}

WResult WEditorTestGenerateCompilePacMan::InitializeSubTest(WInt32 iIdentifier)
{
  return W_SUCCESS;
}

WResult WEditorTestGenerateCompilePacMan::DeInitializeSubTest(WInt32 iIdentifier)
{
  WDocumentManager::CloseAllDocuments();
  return W_SUCCESS;
}
