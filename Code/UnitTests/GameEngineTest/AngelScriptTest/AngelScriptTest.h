#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestApplication_AngelScript : public WGameEngineTestApplication
{
public:
  WGameEngineTestApplication_AngelScript();

  void SubTestBasicsSetup();
  WTestAppRun SubTestBasisExec(const char* szSubTestName);
  /// Creates a module out of the given code and calls the function `void ExecuteTests()` in it.
  void RunTestScript(WStringView sScriptPath);
  void TestScriptExceptionCallback(asIScriptContext* pContext);

private:
  WStringBuilder m_sCode;
  WDynamicArray<WStringView> m_Lines;
};

class WGameEngineTestAngelScript : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

  enum SubTests
  {
    Types,
    Strings,
    Arrays,
    EntryPoints,
    World,
    Messaging,
    EventMessaging,
    GameObject,
    Physics,
    Misc,
  };

private:
  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WGameEngineTestApplication_AngelScript* m_pOwnApplication = nullptr;
};
