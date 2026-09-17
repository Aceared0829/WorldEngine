#include <GameEngineTest/GameEngineTestPCH.h>

#include "RmlUiTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static WGameEngineTestRmlUi s_GameEngineTestAnimations;

const char* WGameEngineTestRmlUi::GetTestName() const
{
  return "RmlUi Tests";
}

WGameEngineTestApplication* WGameEngineTestRmlUi::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "RmlUi");
  return m_pOwnApplication;
}

void WGameEngineTestRmlUi::SetupSubTests()
{
  AddSubTest("Demo", SubTests::Demo);
}

WResult WGameEngineTestRmlUi::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Demo)
  {
    m_ImgCompFrames.PushBack(2);
    m_ImgCompFrames.PushBack(4);
    m_ImgCompFrames.PushBack(7);
    m_ImgCompFrames.PushBack(10);
    m_ImgCompFrames.PushBack(13);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("RmlUi/AssetCache/Common/Scenes/Demo.WBinScene"));
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestRmlUi::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  const bool bVulkan = WGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return WTestAppRun::Quit;
  }

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    W_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}
