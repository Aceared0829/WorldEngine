#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../GenerateCompile/GenerateCompile.h"

class WEditorTestSamples : public WEditorTestGenerateCompile
{
public:
  using SUPER = WEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_Asteroids,
    ST_PacMan,
    ST_RTS,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
};
