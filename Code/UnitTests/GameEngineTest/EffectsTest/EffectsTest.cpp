#include <GameEngineTest/GameEngineTestPCH.h>

#include "EffectsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <ParticlePlugin/Components/ParticleComponent.h>


static WGameEngineTestEffects s_GameEngineTestEffects;

const char* WGameEngineTestEffects::GetTestName() const
{
  return "Effects Tests";
}

WGameEngineTestApplication* WGameEngineTestEffects::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "Effects");
  return m_pOwnApplication;
}

void WGameEngineTestEffects::SetupSubTests()
{
  AddSubTest("Decals", SubTests::Decals);
  AddSubTest("Heightfield", SubTests::Heightfield);
  AddSubTest("WindClothRopes", SubTests::WindClothRopes);
  AddSubTest("Reflections", SubTests::Reflections);
  AddSubTest("StressTest", SubTests::StressTest);
  AddSubTest("AdvancedMeshes", SubTests::AdvancedMeshes);
  AddSubTest("Lighting", SubTests::Lighting);
  AddSubTest("MeshDecals", SubTests::MeshDecals);
}

WResult WGameEngineTestEffects::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  switch (iIdentifier)
  {
    case SubTests::Decals:
    {
      m_ImgCompFrames.PushBack({5});
      m_ImgCompFrames.PushBack({30});
      m_ImgCompFrames.PushBack({60});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Decals.WBinScene");
    }

    case SubTests::Heightfield:
    {
      m_ImgCompFrames.PushBack({20});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Heightfield.WBinScene");
    }

    case SubTests::WindClothRopes:
    {
      m_ImgCompFrames.PushBack({20, 550});

      // TODO: This needs to be investigated why the result is so different on platforms other than windows.
#if W_ENABLED(W_PLATFORM_WINDOWS)
      m_ImgCompFrames.PushBack({100, 850});
#else
      m_ImgCompFrames.PushBack({100, 2000});
#endif

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Wind.WBinScene");
    }

    case SubTests::Reflections:
    {
      m_ImgCompFrames.PushBack({8});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Reflections.WBinScene");
    }

    case SubTests::StressTest:
    {
      m_ImgCompFrames.PushBack({100});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/StressTest.WBinScene");
    }

    case SubTests::AdvancedMeshes:
    {
      m_ImgCompFrames.PushBack({20});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/AdvancedMeshes.WBinScene");
    }

    case SubTests::Lighting:
    {
      m_ImgCompFrames.PushBack({20});
      m_ImgCompFrames.PushBack({40});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Lighting.WBinScene");
    }

    case SubTests::MeshDecals:
    {
      m_ImgCompFrames.PushBack({5, 150});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/MeshDecals.WBinScene");
    }

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestEffects::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
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
      if (false)
      {
        WStringBuilder sPath(":appdata/Profiling/", WApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("effectsProfiling.json");
        WProfilingUtils::SaveProfilingCapture(sPath).IgnoreResult();
      }
      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}
