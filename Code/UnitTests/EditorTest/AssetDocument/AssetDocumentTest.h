#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class WEditorAssetDocumentTest : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_AsyncSave,
    ST_SaveOnTransform,
    ST_FileOperations,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void AsyncSave();
  void SaveOnTransform();
  void FileOperations();
};
