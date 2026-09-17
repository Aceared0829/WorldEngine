#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>
#include <TestFramework/Utilities/TestLogInterface.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestVisualScript : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Variables,
    Variables2,
    Coroutines,
    Messages,
    EnumsAndSwitch,
    Blackboard,
    Loops,
    Loops2,
    Loops3,
    Properties,
    Arrays,
    Maps,
    Expressions,
    Physics,
    Misc,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WInt32 m_iFrame = 0;
  WGameEngineTestApplication* m_pOwnApplication = nullptr;

  WUInt32 m_uiImgCompIdx = 0;
  WHybridArray<WUInt32, 8> m_ImgCompFrames;

  struct TestLog
  {
    WTestLogInterface m_Interface;
    WTestLogSystemScope m_Scope;

    TestLog()
      : m_Scope(&m_Interface, true)
    {
    }
  };

  WUniquePtr<TestLog> m_pTestLog;
};
