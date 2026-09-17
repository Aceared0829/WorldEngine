#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestStateMachine : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Builtins,
    SimpleTransitions,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void RunBuiltinsTest();

  WInt32 m_iFrame = 0;
  WGameEngineTestApplication* m_pOwnApplication = nullptr;

  WUInt32 m_uiImgCompIdx = 0;
  WHybridArray<WUInt32, 8> m_ImgCompFrames;
};
