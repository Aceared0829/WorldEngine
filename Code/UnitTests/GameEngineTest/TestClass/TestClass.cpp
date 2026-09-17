#include <GameEngineTest/GameEngineTestPCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererFoundation/Device/Device.h>

WGameEngineTest::WGameEngineTest() = default;
WGameEngineTest::~WGameEngineTest() = default;

WResult WGameEngineTest::GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber)
{
  ref_img.ResetAndCopy(m_pApplication->GetLastScreenshot());

  return W_SUCCESS;
}

WResult WGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return W_FAILURE;


  W_SUCCEED_OR_RETURN(WRun_Startup(m_pApplication));

  WStringView sAdapterName;
  if (WGALDevice::HasDefaultDevice())
  {
    sAdapterName = WGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName;
  }
  WTestFramework::GetInstance()->SetImageReferenceTagsFromEnvironment(W_PLATFORM_NAME, WGameApplication::GetActiveRenderer(), sAdapterName);

  return W_SUCCESS;
}

WResult WGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->QuitApplication();

    WInt32 iSteps = 2;
    while (!m_pApplication->ShouldApplicationQuit() && iSteps > 0)
    {
      m_pApplication->Run();
      --iSteps;
    }

    WRun_Shutdown(m_pApplication);

    W_DEFAULT_DELETE(m_pApplication);

    if (iSteps == 0)
      return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WGameEngineTest::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  WResourceManager::ForceNoFallbackAcquisition(3);

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


WGameEngineTestApplication::WGameEngineTestApplication(const char* szProjectDirName)
  : WGameApplication("WGameEngineTest", nullptr)
{
  m_pWorld = nullptr;
  m_sProjectDirName = szProjectDirName;
}


WString WGameEngineTestApplication::FindProjectDirectory() const
{
  return m_sAppProjectPath;
}

WString WGameEngineTestApplication::GetProjectDataDirectoryPath() const
{
  WStringBuilder sProjectPath(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath(), "/", m_sProjectDirName);
  return sProjectPath;
}

void WGameEngineTestApplication::SwitchToCamera(WUInt32 uiCameraNumber)
{
  WWorld* pWorld = GetWorld();
  W_LOCK(pWorld->GetReadMarker());
  WGameObject* pCamera = nullptr;
  WStringBuilder sCamera;
  sCamera.SetFormat("Camera{}", uiCameraNumber);
  if (pWorld->TryGetObjectWithGlobalKey(WTempHashedString(sCamera), pCamera))
  {
    WCameraComponent* pCameraComponent = nullptr;
    if (pCamera->TryGetComponentOfBaseType<WCameraComponent>(pCameraComponent))
    {
      // update view camera
      WCamera* pCamera = WDynamicCast<WGameState*>(GetActiveGameState())->GetMainCamera();
      const WGameObject* pOwner = pCameraComponent->GetOwner();
      const WVec3 vPos = pOwner->GetGlobalPosition();
      const WVec3 vFwd = pOwner->GetGlobalDirForwards();
      const WVec3 vUp = pOwner->GetGlobalDirUp();
      pCamera->LookAt(vPos, vPos + vFwd, vUp);
      pCamera->SetCameraMode(pCameraComponent->GetCameraMode(), pCameraComponent->GetFieldOfView(), pCameraComponent->GetNearPlane(), pCameraComponent->GetFarPlane());
    }
  }
}

WResult WGameEngineTestApplication::LoadScene(const char* szSceneFile)
{
  W_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();
  m_pWorld->GetRandomNumberGenerator().Initialize(42); // reset the RNG
  m_pWorld->GetClock().Reset(false);                   // reset the world clock

  WFileReader file;

  if (file.Open(szSceneFile).Succeeded())
  {
    // File Header
    {
      WAssetFileHeader header;
      W_SUCCEED_OR_RETURN(header.Read(file));

      char szSceneTag[16];
      file.ReadBytes(szSceneTag, sizeof(char) * 16);

      W_ASSERT_RELEASE(WStringUtils::IsEqualN(szSceneTag, "[WEBinaryScene]", 16), "The given file is not a valid scene file");
    }

    WWorldReader reader;
    W_SUCCEED_OR_RETURN(reader.ReadWorldDescription(file));
    reader.InstantiateWorld(*m_pWorld, nullptr);

    return W_SUCCESS;
  }
  else
  {
    WLog::Error("Failed to load scene '{0}'", szSceneFile);
    return W_FAILURE;
  }
}

WResult WGameEngineTestApplication::BeforeCoreSystemsStartup()
{
  W_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  WStringBuilder sProject;
  W_SUCCEED_OR_RETURN(WFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sProject));
  m_sAppProjectPath = sProject;

  return W_SUCCESS;
}


void WGameEngineTestApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  WStartup::StartupHighLevelSystems();

  WWorldDesc desc("GameEngineTestWorld");
  desc.m_uiRandomNumberGeneratorSeed = 42;

  m_pWorld = W_DEFAULT_NEW(WWorld, desc);
  m_pWorld->GetClock().SetFixedTimeStep(WTime::MakeFromSeconds(1.0 / 30.0));
  // Disable VSync. Tests run at a fixed time step so this makes tests much faster without changing the outcome.
  WGameApplication::cvar_AppVSync = false;

  ActivateGameState(m_pWorld.Borrow(), {}, WTransform::MakeIdentity());
}

void WGameEngineTestApplication::BeforeHighLevelSystemsShutdown()
{
  m_pWorld = nullptr;

  SUPER::BeforeHighLevelSystemsShutdown();
}

void WGameEngineTestApplication::StoreScreenshot(WImage&& image, WStringView sContext)
{
  // store this for image comparison purposes
  m_LastScreenshot.ResetAndMove(std::move(image));
}


void WGameEngineTestApplication::Init_FileSystem_ConfigureDataDirs()
{
  SUPER::Init_FileSystem_ConfigureDataDirs();

  // additional data directories for the tests to work
  {
    WFileSystem::SetSpecialDirectory("testout", WTestFramework::GetInstance()->GetAbsOutputPath());

    WStringBuilder sBaseDir = ">sdk/Data/Base/";
    WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());

    WFileSystem::AddDataDirectory(">Wtest/", "ImageComparisonDataDir", "imgout", WDataDirUsage::AllowWrites).IgnoreResult();
    WFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
  }
}

WUniquePtr<WGameStateBase> WGameEngineTestApplication::CreateGameState()
{
  return W_DEFAULT_NEW(WGameEngineTestGameState);
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameEngineTestGameState, 1, WRTTIDefaultAllocator<WGameEngineTestGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

void WGameEngineTestGameState::ProcessInput()
{
  // Do nothing, user input should be ignored

  // trigger taking a screenshot every frame, for image comparison purposes
  WGameApplicationBase::GetGameApplicationBaseInstance()->TakeScreenshot();
}

void WGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
