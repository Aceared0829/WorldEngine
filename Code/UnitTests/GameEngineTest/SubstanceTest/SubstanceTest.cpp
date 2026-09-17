#include <GameEngineTest/GameEngineTestPCH.h>

#include "SubstanceTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include <optional>

static WGameEngineTestSubstance s_GameEngineTestAnimations;

const char* WGameEngineTestSubstance::GetTestName() const
{
  return "Substance Tests";
}

WGameEngineTestApplication* WGameEngineTestSubstance::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "Substance");
  return m_pOwnApplication;
}

// static
bool WGameEngineTestSubstance::HasSubstanceDesignerInstalled()
{
#if W_ENABLED(W_PLATFORM_WINDOWS)
  static std::optional<bool> s_Cache;
  if (s_Cache.has_value())
  {
    return *s_Cache;
  }

  auto CheckPath = [&](WStringView sPath)
  {
    WStringBuilder path = sPath;
    path.AppendPath("sbscooker.exe");

    if (WOSFile::ExistsFile(path))
    {
      s_Cache = true;
      return true;
    }

    return false;
  };

  WStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
  if (CheckPath(sPath))
    return true;

  s_Cache = false;
  return false;
#else
  return false;
#endif
}

void WGameEngineTestSubstance::SetupSubTests()
{
  AddSubTest("Basics", SubTests::Basics);
}

WResult WGameEngineTestSubstance::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  if (HasSubstanceDesignerInstalled() == false)
  {
    WLog::Warning("Substance Designer is not installed. Skipping test.");
    return W_SUCCESS;
  }

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Basics)
  {
    m_ImgCompFrames.PushBack(9);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Substance/AssetCache/Common/Scenes/Substance.WBinScene"));
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestSubstance::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  if (HasSubstanceDesignerInstalled() == false)
  {
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
