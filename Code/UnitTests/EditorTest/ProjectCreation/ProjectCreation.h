#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

/// Creates projects through 'WEditorProcessor -createProject' and transforms the result.
///
/// Everything runs in a separate process, because creating a project means loading a different set of
/// plugins, which an already running editor cannot do. The transform sub-test is what notices a project
/// template that stopped working - those are otherwise only exercised when someone creates a project by hand.
class WEditorTestProjectCreation : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateBlankProject,
    ST_CreateBasicFpsProject,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  /// Absolute path of a folder below the test output directory.
  void GetTargetFolder(WStringBuilder& out_sPath, WStringView sName) const;
  /// Same, but deletes the folder if it exists, so that a project is always created from scratch.
  WResult PrepareTargetFolder(WStringBuilder& out_sPath, WStringView sName);

  WStatus CreateBlankProject();
  WStatus CreateProjectFromTemplate(WStringView sTemplate);
  WStatus TransformTemplateProject(WStringView sTemplate);

  WString m_sTemplateProjectPath;
};
