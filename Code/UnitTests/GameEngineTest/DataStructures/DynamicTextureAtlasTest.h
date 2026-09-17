#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <RendererCore/Textures/DynamicTextureAtlas.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestDynamicTextureAtlas : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ST_AllocationsSmall,
    ST_AllocationsLarge,
    ST_AllocationsMixed,
    ST_Deallocations,
    ST_Deallocations2,
  };

  virtual void SetupSubTests() override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

private:
  WInt32 m_iFrame = 0;
  WGameEngineTestApplication* m_pOwnApplication = nullptr;

  WDynamicTextureAtlas m_TextureAtlas;
};
