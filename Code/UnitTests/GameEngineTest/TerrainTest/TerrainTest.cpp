#include <GameEngineTest/GameEngineTestPCH.h>

#include "TerrainTest.h"

static WGameEngineTestTerrain s_GameEngineTestEffects;

const char* WGameEngineTestTerrain::GetTestName() const
{
  return "Terrain Tests";
}

WGameEngineTestApplication* WGameEngineTestTerrain::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "Terrain");
  return m_pOwnApplication;
}

void WGameEngineTestTerrain::SetupSubTests()
{
  AddSubTest("Heightfields", SubTests::HeightfieldTerrain);
  AddSubTest("Voxels", SubTests::VoxelTerrain);
}

WResult WGameEngineTestTerrain::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  switch (iIdentifier)
  {
    case SubTests::HeightfieldTerrain:
    {
      // due to how the system works, frame 3 is the first one that will see terrain
      m_ImgCompFrames.PushBack({3});

      return m_pOwnApplication->LoadScene("Terrain/AssetCache/Common/Scenes/Heightfields.WBinScene");
    }

    case SubTests::VoxelTerrain:
    {
      // due to how the system works, frame 3 is the first one that will see terrain
      m_ImgCompFrames.PushBack({3});

      return m_pOwnApplication->LoadScene("Terrain/AssetCache/Common/Scenes/Voxels.WBinScene");
    }

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestTerrain::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return WTestAppRun::Quit;

  m_pOwnApplication->SwitchToCamera(m_iFrame);

  if (m_ImgCompFrames[m_uiImgCompIdx].m_uiFrame == m_iFrame)
  {
    W_TEST_IMAGE(m_uiImgCompIdx, m_ImgCompFrames[m_uiImgCompIdx].m_uiThreshold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}
