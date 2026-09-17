#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestApplication_Basics : public WGameEngineTestApplication
{
public:
  WGameEngineTestApplication_Basics();

  void SubTestManyMeshesSetup();
  WTestAppRun SubTestManyMeshesExec(WInt32 iCurFrame);

  void SubTestSkyboxSetup();
  WTestAppRun SubTestSkyboxExec(WInt32 iCurFrame);

  void SubTestDebugRenderingSetup();
  WTestAppRun SubTestDebugRenderingExec(WInt32 iCurFrame);

  WTestAppRun SubTestDebugRenderingExec2(WInt32 iCurFrame);

  void SubTestLoadSceneSetup();
  WTestAppRun SubTestLoadSceneExec(WInt32 iCurFrame);

  void SubTestGoReferenceSetup();
  WTestAppRun SubTestGoReferenceExec(WInt32 iCurFrame);
};

class WGameEngineTestBasics : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ManyMeshes,
    Skybox,
    DebugRendering,
    DebugRendering2,
    LoadScene,
    GameObjectReferences,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WInt32 m_iFrame;
  WGameEngineTestApplication_Basics* m_pOwnApplication;
};
