#include <GameEngineTest/GameEngineTestPCH.h>

#include "AnimationsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static WGameEngineTestAnimations s_GameEngineTestAnimations;

const char* WGameEngineTestAnimations::GetTestName() const
{
  return "Animations Tests";
}

WGameEngineTestApplication* WGameEngineTestAnimations::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "Animations");
  return m_pOwnApplication;
}

void WGameEngineTestAnimations::SetupSubTests()
{
  AddSubTest("Skeletal", SubTests::Skeletal);
  AddSubTest("CurveData", SubTests::CurveData);
}

WResult WGameEngineTestAnimations::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Skeletal)
  {
    m_ImgCompFrames.PushBack(10);
    m_ImgCompFrames.PushBack(30);
    m_ImgCompFrames.PushBack(60);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Animations/AssetCache/Common/Scenes/AnimController.WBinScene"));
    return W_SUCCESS;
  }

  if (iIdentifier == SubTests::CurveData)
  {
    m_ImgCompFrames.PushBack(15);
    m_ImgCompFrames.PushBack(75);
    m_ImgCompFrames.PushBack(100);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Animations/AssetCache/Common/Scenes/AnimCurves.WBinScene"));
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestAnimations::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ++m_iFrame;

  m_pOwnApplication->Run();

  if (m_pOwnApplication->ShouldApplicationQuit())
    return WTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    W_TEST_IMAGE(m_uiImgCompIdx, 300);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}
