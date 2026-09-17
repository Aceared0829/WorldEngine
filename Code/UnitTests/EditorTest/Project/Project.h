#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class WEditorTestProject : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateDocuments,
    ST_CreateCppSolution
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WTestAppRun CreateDocuments();
  WTestAppRun CreateCppSolution();
};
