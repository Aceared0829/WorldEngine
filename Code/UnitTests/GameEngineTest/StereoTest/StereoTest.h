#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/TestClass/TestClass.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)

class WStereoTestGameState : public WGameEngineTestGameState
{
  W_ADD_DYNAMIC_REFLECTION(WStereoTestGameState, WGameEngineTestGameState);

public:
  void OverrideRenderPipeline(WTypedResourceHandle<WRenderPipelineResource> hPipeline);
};

class WStereoTestApplication : public WGameEngineTestApplication
{
public:
  using SUPER = WGameEngineTestApplication;

  WStereoTestApplication(const char* szProjectDirName);
  WPlatformProfile& GetPlatformProfile() { return m_PlatformProfile; }

protected:
  virtual WUniquePtr<WGameStateBase> CreateGameState() override;
};


class WStereoTest : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    HoloLensPipeline,
    DefaultPipeline
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WInt32 m_iFrame = 0;
  WStereoTestApplication* m_pOwnApplication = nullptr;

  WUInt32 m_uiImgCompIdx = 0;
  WHybridArray<WUInt32, 8> m_ImgCompFrames;
};

#endif
