#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static WGameEngineTestStateMachine s_GameEngineTestAnimations;

const char* WGameEngineTestStateMachine::GetTestName() const
{
  return "StateMachine Tests";
}

WGameEngineTestApplication* WGameEngineTestStateMachine::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "StateMachine");
  return m_pOwnApplication;
}

void WGameEngineTestStateMachine::SetupSubTests()
{
  AddSubTest("Builtins", SubTests::Builtins);
  AddSubTest("SimpleTransitions", SubTests::SimpleTransitions);
}

WResult WGameEngineTestStateMachine::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Builtins)
  {
    return W_SUCCESS;
  }
  else if (iIdentifier == SubTests::SimpleTransitions)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(17);
    m_ImgCompFrames.PushBack(34);
    m_ImgCompFrames.PushBack(51);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("StateMachine/AssetCache/Common/Scenes/StateMachine.WBinScene"));
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestStateMachine::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::Builtins)
  {
    RunBuiltinsTest();
    return WTestAppRun::Quit;
  }

  const bool bVulkan = WGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return WTestAppRun::Quit;

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
