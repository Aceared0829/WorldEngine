#include <EditorTest/EditorTestPCH.h>

#include "Samples.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <TestFramework/Framework/TestFramework.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
// only show/enable this test on Windows, since we can only compile game plugins there
static WEditorTestSamples s_EditorTestSamples;
#endif

const char* WEditorTestSamples::GetTestName() const
{
  return "Samples";
}

void WEditorTestSamples::SetupSubTests()
{
  AddSubTest("Asteroids", SubTests::ST_Asteroids);
  AddSubTest("PacMan", SubTests::ST_PacMan);
  AddSubTest("RTS", SubTests::ST_RTS);
}

WResult WEditorTestSamples::InitializeTest()
{
  return W_SUCCESS;
}

WResult WEditorTestSamples::DeInitializeTest()
{
  return W_SUCCESS;
}

WResult WEditorTestSamples::InitializeSubTest(WInt32 iIdentifier)
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  switch (iIdentifier)
  {
    case SubTests::ST_PacMan:
      m_sProjectName = "PacMan";
      break;
    case SubTests::ST_Asteroids:
      m_sProjectName = "Asteroids";
      break;
    case SubTests::ST_RTS:
      m_sProjectName = "RTS";
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  WStringBuilder sPath("Data/Samples/", m_sProjectName);

  if (SUPER::OpenProject(sPath).Failed())
    return W_FAILURE;

  if (WCppProject::ForceSdkCompatibleCompiler().Failed())
  {
    WLog::Error("Failed to autodetect SDK compatible compiler for testing");
    return W_FAILURE;
  }

  WCppSettings cpp;
  cpp.Load().AssertSuccess("Failed to load C++ project settings");

  // update sources, in case there was a version change
  WUInt32 uiNumFilesCopied = 0;
  WCppProject::PopulateWithDefaultSources(cpp, &uiNumFilesCopied).AssertSuccess();


  W_TEST_INT_MSG(uiNumFilesCopied, 0, "You need to open the '%s' sample and regenerate the C++ solution.", m_sProjectName.GetData());

  WPreferences::SaveApplicationPreferences();

  return W_SUCCESS;
}

WResult WEditorTestSamples::DeInitializeSubTest(WInt32 iIdentifier)
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    WStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    WOSFile::DeleteFolder(buildOutput).IgnoreResult();
    m_sProjectPath.Clear();
  }

  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}


WTestAppRun WEditorTestSamples::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ProcessEvents();

  W_TEST_STATUS(GenerateAndCompile());

  ProcessEvents();
  return WTestAppRun::Quit;
}
