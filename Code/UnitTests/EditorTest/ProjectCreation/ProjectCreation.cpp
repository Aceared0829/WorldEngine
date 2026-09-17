#include <EditorTest/EditorTestPCH.h>

#include "ProjectCreation.h"
#include <Foundation/IO/OSFile.h>
#include <TestFramework/Framework/TestFramework.h>

static WEditorTestProjectCreation s_EditorTestProjectCreation;

const char* WEditorTestProjectCreation::GetTestName() const
{
  return "Project Creation";
}

void WEditorTestProjectCreation::SetupSubTests()
{
  AddSubTest("01 - Create Blank Project", SubTests::ST_CreateBlankProject);
  AddSubTest("02 - Create 'Basic FPS' Project", SubTests::ST_CreateBasicFpsProject);
}

WResult WEditorTestProjectCreation::InitializeTest()
{
  // no project is opened here - every sub-test creates its own in a separate process
  return SUPER::InitializeTest();
}

WResult WEditorTestProjectCreation::DeInitializeTest()
{
  return SUPER::DeInitializeTest();
}

WTestAppRun WEditorTestProjectCreation::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  WStatus res(W_SUCCESS);

  switch (iIdentifier)
  {
    case ST_CreateBlankProject:
      res = CreateBlankProject();
      break;
    case ST_CreateBasicFpsProject:
      res = TransformTemplateProject("Basic FPS");
      break;
    default:
      W_REPORT_FAILURE("missing case statement");
      break;
  }

  if (res.Failed())
  {
    WTestFramework::GetInstance()->Error(res.GetMessageString(), W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION, "");
  }

  return WTestAppRun::Quit;
}

void WEditorTestProjectCreation::GetTargetFolder(WStringBuilder& out_sPath, WStringView sName) const
{
  out_sPath = WTestFramework::GetInstance()->GetAbsOutputPath();
  out_sPath.AppendPath(GetTestName(), sName);
  out_sPath.MakeCleanPath();
}

WResult WEditorTestProjectCreation::PrepareTargetFolder(WStringBuilder& out_sPath, WStringView sName)
{
  GetTargetFolder(out_sPath, sName);

  if (WOSFile::ExistsDirectory(out_sPath) && WOSFile::DeleteFolder(out_sPath).Failed())
  {
    WLog::Error("Failed to delete the previous test project '{}'.", out_sPath);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WStatus WEditorTestProjectCreation::CreateBlankProject()
{
  WStringBuilder sProjectPath;
  if (PrepareTargetFolder(sProjectPath, "BlankProject").Failed())
    return WStatus("Failed to prepare the target folder");

  WDynamicArray<WString> arguments;
  arguments.PushBack("-createProject");
  arguments.PushBack(sProjectPath);
  arguments.PushBack("-pluginTemplate");
  arguments.PushBack("General3D");

  W_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // the plugin selection is the only thing the creation writes for a blank project, the 'WProject'
  // file is written when the new project is opened afterwards - both have to be there
  WStringBuilder sFile(sProjectPath, "/Editor/PluginSelection.ddl");
  W_TEST_BOOL_MSG(WOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  sFile.Set(sProjectPath, "/WProject");
  W_TEST_BOOL_MSG(WOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  return W_SUCCESS;
}

WStatus WEditorTestProjectCreation::CreateProjectFromTemplate(WStringView sTemplate)
{
  WStringBuilder sProjectPath;
  if (PrepareTargetFolder(sProjectPath, "TemplateProject").Failed())
    return WStatus("Failed to prepare the target folder");

  WDynamicArray<WString> arguments;
  arguments.PushBack("-createProject");
  arguments.PushBack(sProjectPath);
  arguments.PushBack("-projectTemplate");
  arguments.PushBack(sTemplate);

  W_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  WStringBuilder sFile(sProjectPath, "/WProject");
  W_TEST_BOOL_MSG(WOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  // the template brings its own plugin selection, and content that a blank project does not have
  sFile.Set(sProjectPath, "/Editor/PluginSelection.ddl");
  W_TEST_BOOL_MSG(WOSFile::ExistsFile(sFile), "'%s' was not copied from the template", sFile.GetData());

  sFile.Set(sProjectPath, "/Scenes");
  W_TEST_BOOL_MSG(WOSFile::ExistsDirectory(sFile), "'%s' was not copied from the template", sFile.GetData());

  m_sTemplateProjectPath = sProjectPath;
  return W_SUCCESS;
}

WStatus WEditorTestProjectCreation::TransformTemplateProject(WStringView sTemplate)
{
  // so that this sub-test can also run on its own, without the one that creates the project
  if (m_sTemplateProjectPath.IsEmpty())
  {
    W_SUCCEED_OR_RETURN(CreateProjectFromTemplate(sTemplate));
  }

  WDynamicArray<WString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sTemplateProjectPath);
  arguments.PushBack("-transform");
  arguments.PushBack("Default");
  arguments.PushBack("-outputDir");

  WStringBuilder sUserDataDir = WTestFramework::GetInstance()->GetAbsOutputPath();
  sUserDataDir.AppendPath(GetTestName());
  arguments.PushBack(sUserDataDir);

  W_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  return WStatus(W_SUCCESS);
}
