#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class WDocument;

class WEditorTestGenerateCompile : public WEditorTest
{
public:
  using SUPER = WEditorTest;

protected:
  WStatus PrepareCompile(WStringBuilder& dllPath);

  WStatus GenerateAndCompile();
  WStatus EditorProcessorCompileOnly();
  WStatus EditorProcessorCompileAndTransform();

  WString m_sProjectName;
  WDocument* m_pDocument = nullptr;
};

class WEditorTestGenerateCompilePacMan : public WEditorTestGenerateCompile
{
public:
  using SUPER = WEditorTestGenerateCompile;

  WEditorTestGenerateCompilePacMan()
  {
    m_sProjectName = "PacMan";
  }

  virtual const char* GetTestName() const override;

private:
  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  enum SubTests
  {
    ST_GenerateAndCompile,
    ST_EditorProcessorCompileOnly,
    ST_EditorProcessorCompileAndTransform
  };
};
