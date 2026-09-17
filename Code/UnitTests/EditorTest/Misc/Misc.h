#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class WDocument;

class WEditorTestMisc : public WEditorTest
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    GameObjectReferences,
    DefaultValues,
    AssetBrowerModel,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WTestAppRun GameObjectReferencesTest();
  WTestAppRun DefaultValuesTest();
  WTestAppRun AssetBrowerModelTest();

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  WDocument* m_pDocument = nullptr;
};
