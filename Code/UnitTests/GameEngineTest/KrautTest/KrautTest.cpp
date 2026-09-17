#include <GameEngineTest/GameEngineTestPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)

#  include "KrautTest.h"
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <KrautPlugin/Components/KrautTreeComponent.h>
#  include <ParticlePlugin/Components/ParticleComponent.h>

static WGameEngineTestKraut s_GameEngineTestAnimations;

const char* WGameEngineTestKraut::GetTestName() const
{
  return "Kraut Tests";
}

WGameEngineTestApplication* WGameEngineTestKraut::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "PlatformWin");
  return m_pOwnApplication;
}

void WGameEngineTestKraut::SetupSubTests()
{
  AddSubTest("TreeRendering", SubTests::TreeRendering);
}

WResult WGameEngineTestKraut::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::TreeRendering)
  {
    m_ImgCompFrames.PushBack(3);
    m_ImgCompFrames.PushBack(60);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("PlatformWin/AssetCache/Common/Kraut/Kraut.WBinScene"));

    // Force synchronous tree generation so image comparisons are deterministic.
    // Without this, async generation means the tree may not be visible at frame 3
    // and wind accumulation differs from the reference images.
    {
      WWorld* pWorld = m_pOwnApplication->GetWorld();
      W_LOCK(pWorld->GetWriteMarker());
      WKrautTreeComponentManager* pManager = pWorld->GetOrCreateComponentManager<WKrautTreeComponentManager>();
      for (auto it = pManager->GetComponents(); it.IsValid(); it.Next())
      {
        it->m_bForceGenerateImmediate = true;
      }
    }

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestKraut::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return WTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    W_TEST_IMAGE(m_uiImgCompIdx, 450);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}

#endif
