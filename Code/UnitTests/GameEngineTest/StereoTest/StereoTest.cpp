#include <GameEngineTest/GameEngineTestPCH.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)

#  include <Core/World/World.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Foundation/Profiling/ProfilingUtils.h>
#  include <GameEngineTest/StereoTest/StereoTest.h>
#  include <RendererCore/Components/CameraComponent.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererFoundation/Device/Device.h>

static WStereoTest s_StereoTest;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStereoTestGameState, 1, WRTTIDefaultAllocator<WStereoTestGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

void WStereoTestGameState::OverrideRenderPipeline(WTypedResourceHandle<WRenderPipelineResource> hPipeline)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetRenderPipelineResource(hPipeline);
  }
}

//////////////////////////////////////////////////////////////////////////

WStereoTestApplication::WStereoTestApplication(const char* szProjectDirName)
  : WGameEngineTestApplication(szProjectDirName)
{
}

WUniquePtr<WGameStateBase> WStereoTestApplication::CreateGameState()
{
  return W_DEFAULT_NEW(WStereoTestGameState);
}

//////////////////////////////////////////////////////////////////////////

const char* WStereoTest::GetTestName() const
{
  return "Stereo Test";
}

WGameEngineTestApplication* WStereoTest::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WStereoTestApplication, "XR");
  return m_pOwnApplication;
}

void WStereoTest::SetupSubTests()
{
  AddSubTest("HoloLensPipeline", SubTests::HoloLensPipeline);
  AddSubTest("DefaultPipeline", SubTests::DefaultPipeline);
}

WResult WStereoTest::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::HoloLensPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.WBinScene"));

    auto renderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    WDynamicCast<WStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return W_SUCCESS;
  }
  if (iIdentifier == SubTests::DefaultPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    W_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.WBinScene"));

    auto renderPipeline = WResourceManager::LoadResource<WRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    WDynamicCast<WStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WStereoTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ++m_iFrame;
  WStringBuilder sb;
  sb.SetFormat("{}", m_iFrame);

  WDebugRenderer::Draw2DText(m_pApplication->GetWorld(), sb, WVec2I32(50, 50), WColor::Brown, 60);

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return WTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    // The particle effect increases the error on lavapipe, see WGameEngineTestParticles::GetImageCompareThreshold.
    WUInt32 uiThreshhold = WGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe") ? 300 : 250;
    W_TEST_IMAGE(m_uiImgCompIdx, uiThreshhold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        WStringBuilder sPath(":appdata/Profiling/", WApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("stereoProfiling.json");
        WProfilingUtils::SaveProfilingCapture(sPath).IgnoreResult();
      }

      return WTestAppRun::Quit;
    }
  }

  return WTestAppRun::Continue;
}

#endif
