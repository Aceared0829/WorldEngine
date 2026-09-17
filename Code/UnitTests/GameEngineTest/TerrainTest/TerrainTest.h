#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestTerrain : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    HeightfieldTerrain,
    VoxelTerrain,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WInt32 m_iFrame = 0;
  WGameEngineTestApplication* m_pOwnApplication = nullptr;

  WUInt32 m_uiImgCompIdx = 0;

  struct ImgCompare
  {
    ImgCompare(WUInt32 uiFrame, WUInt32 uiThreshold = 450)
    {
      m_uiFrame = uiFrame;
      m_uiThreshold = uiThreshold;
    }

    WUInt32 m_uiFrame;
    WUInt32 m_uiThreshold = 450;
  };

  WHybridArray<ImgCompare, 8> m_ImgCompFrames;
};
