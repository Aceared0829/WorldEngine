#include <GameEngineTest/GameEngineTestPCH.h>

#include "Basics.h"
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <GameEngineTest/SubstanceTest/SubstanceTest.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES)
WResult TranformProject(const char* szProjectPath, WUInt32 uiCleanVersion)
{
  WGlobalLog::AddLogWriter(&WLogWriter::Console::LogMessageHandler);
  W_SCOPE_EXIT(WGlobalLog::RemoveLogWriter(&WLogWriter::Console::LogMessageHandler));

  WStringBuilder sBinPath = WOSFile::GetApplicationDirectory();

  WStringBuilder sProjectDir;
  if (WPathUtils::IsAbsolutePath(szProjectPath))
  {
    sProjectDir = szProjectPath;
    sProjectDir.MakeCleanPath();
  }
  else
  {
    // Assume to be relative to W root.
    sProjectDir = WFileSystem::GetSdkRootDirectory();
    sProjectDir.AppendPath(szProjectPath);
    sProjectDir.MakeCleanPath();
  }

  WLog::Info("Transforming assets for project '{}'", sProjectDir);

  {
    WStringBuilder sProjectAssetDir = sProjectDir;
    sProjectAssetDir.PathParentDirectory();
    sProjectAssetDir.AppendPath("AssetCache");

    WStringBuilder sCleanFile = sProjectAssetDir;
    sCleanFile.AppendPath("CleanVersion.dat");

    WUInt32 uiTargetVersion = 0;
    WOSFile f;

    if (f.Open(sCleanFile, WFileOpenMode::Read, WFileShareMode::Default).Succeeded())
    {
      f.Read(&uiTargetVersion, sizeof(WUInt32));
      f.Close();

      WLog::Info("CleanVersion.dat exists -> project has been transformed before.");
    }

    if (uiTargetVersion != uiCleanVersion)
    {
      WLog::Info("Clean version {} != {} -> deleting asset cache.", uiTargetVersion, uiCleanVersion);

#  if (W_ENABLED(W_SUPPORTS_FILE_ITERATORS) && W_ENABLED(W_SUPPORTS_FILE_STATS))
      if (WOSFile::DeleteFolder(sProjectAssetDir).Failed())
      {
        WLog::Warning("Deleting the asset cache folder failed.");
      }
#  endif

      if (f.Open(sCleanFile, WFileOpenMode::Write, WFileShareMode::Default).Succeeded())
      {
        f.Write(&uiCleanVersion, sizeof(WUInt32)).IgnoreResult();
        f.Close();
      }
    }
    else
    {
      WLog::Info("Clean version {} == {}.", uiTargetVersion, uiCleanVersion);
    }
  }


#  if W_ENABLED(W_PLATFORM_WINDOWS)
  sBinPath.AppendPath("WEditorProcessor.exe");
#  else
  sBinPath.AppendPath("WEditorProcessor");
#  endif
  sBinPath.MakeCleanPath();

  WStringBuilder sOutputPath = WTestFramework::GetInstance()->GetAbsOutputPath();
  {
    WStringView sProjectPath = WPathUtils::GetFileDirectory(szProjectPath);
    sProjectPath.Trim("\\/");
    WStringView sProjectName = WPathUtils::GetFileName(sProjectPath);
    sOutputPath.AppendPath("Transform");
    sOutputPath.Append(sProjectName);
    if (WOSFile::CreateDirectoryStructure(sOutputPath).Failed())
      WLog::Error("Failed to create output directory: {}", sOutputPath);
  }

  WStringBuilder sStdout;
  WMutex mutex;

  WProcessOptions opt;
  opt.m_onStdOut = [&sStdout, &mutex](WStringView sView)
  {
    W_LOCK(mutex);
    sStdout.Append(sView);
  };
  opt.m_onStdError = [&sStdout, &mutex](WStringView sView)
  {
    W_LOCK(mutex);
    sStdout.Append(sView);
  };

  opt.m_sProcess = sBinPath;
  opt.m_Arguments.PushBack("-project");
  opt.AddArgument("\"{0}\"", sProjectDir);
  opt.m_Arguments.PushBack("-transform");
  opt.m_Arguments.PushBack("Default");
  opt.m_Arguments.PushBack("-outputDir");
  opt.AddArgument("\"{0}\"", sOutputPath);
  opt.m_Arguments.PushBack("-AssetThumbnails");
  opt.m_Arguments.PushBack("never");
  opt.m_Arguments.PushBack("-renderer");
  opt.m_Arguments.PushBack(WGameApplication::GetActiveRenderer());
  opt.m_Arguments.PushBack("-fullcrashdumps");

  WProcess proc;
  WLog::Info("Launching: '{0}'", sBinPath);
  WResult res = proc.Launch(opt);
  if (res.Failed())
  {
    proc.Terminate().IgnoreResult();
    WLog::Error("Failed to start process: '{0}'", sBinPath);
  }

  WTime timeout = WTime::MakeFromMinutes(15);
  res = proc.WaitToFinish(timeout);
  WResult returnValue = W_SUCCESS;
  if (res.Failed())
  {
#  if W_ENABLED(W_PLATFORM_WINDOWS)
    WStringBuilder sDumpFile = sOutputPath;
    sDumpFile.AppendPath("Timeout.dmp");
    WMiniDumpUtils::WriteExternalProcessMiniDump(sDumpFile, proc.GetProcessID()).LogFailure();
#  endif
    proc.Terminate().IgnoreResult();
    WLog::Error("Process timeout ({1}): '{0}'", sBinPath, timeout);
    returnValue = W_FAILURE;
  }
  else if (proc.GetExitCode() != 0)
  {
    WLog::Error("Process failure ({0}): ExitCode: '{1}'", sBinPath, proc.GetExitCode());
    returnValue = W_FAILURE;
  }
  else
  {
    WLog::Success("Executed Asset Processor to transform '{}'", szProjectPath);
  }

  {
    WOSFile fileWriter;
    WStringBuilder sLogFile = sOutputPath;
    sLogFile.AppendFormat("/EditorProcessor_{}.txt", proc.GetProcessID());
    if (fileWriter.Open(sLogFile, WFileOpenMode::Write, WFileShareMode::Exclusive).Failed() || fileWriter.Write(sStdout.GetData(), sStdout.GetElementCount()).Failed())
    {
      WLog::Error("Failed to write EditorProcessor log at '{}'", sLogFile);
    }
  }

  return returnValue;
}
#endif

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)
W_CREATE_SIMPLE_TEST_GROUP(00_Init);

W_CREATE_SIMPLE_TEST(00_Init, 00_TransformBase) // prefix with 00_ to ensure base data is transformed first
{
  W_TEST_BOOL(TranformProject("Data/Base/WProject", 2).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformBasics)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Basics/WProject", 2).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformParticles)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Particles/WProject", 5).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformEffects)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Effects/WProject", 4).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformAnimations)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Animations/WProject", 7).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformStateMachine)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/StateMachine/WProject", 7).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformPlatformWin)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/PlatformWin/WProject", 7).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformXR)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/XR/WProject", 5).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformVisualScript)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/VisualScript/WProject", 7).Succeeded());
}

W_CREATE_SIMPLE_TEST(00_Init, TransformSubstance)
{
  if (WGameEngineTestSubstance::HasSubstanceDesignerInstalled())
  {
    W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Substance/WProject", 1).Succeeded());
  }
}

W_CREATE_SIMPLE_TEST(00_Init, TransformProcGen)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/ProcGen/WProject", 6).Succeeded());
}

#  ifdef BUILDSYSTEM_ENABLE_RMLUI_SUPPORT
W_CREATE_SIMPLE_TEST(00_Init, TransformRmlUi)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/RmlUi/WProject", 3).Succeeded());
}
#  endif

#  ifdef BUILDSYSTEM_ENABLE_ANGELSCRIPT_SUPPORT
W_CREATE_SIMPLE_TEST(00_Init, TransformAngelScript)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/AngelScript/WProject", 2).Succeeded());
}
#  endif

W_CREATE_SIMPLE_TEST(00_Init, TransformTerrain)
{
  W_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Terrain/WProject", 1).Succeeded());
}

#endif


static WGameEngineTestBasics s_GameEngineTestBasics;

const char* WGameEngineTestBasics::GetTestName() const
{
  return "Basic Engine Tests";
}

WGameEngineTestApplication* WGameEngineTestBasics::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication_Basics);
  return m_pOwnApplication;
}

void WGameEngineTestBasics::SetupSubTests()
{
  AddSubTest("Many Meshes", SubTests::ManyMeshes);
  AddSubTest("Skybox", SubTests::Skybox);
  AddSubTest("Debug Rendering", SubTests::DebugRendering);
  AddSubTest("Debug Rendering - No Lines", SubTests::DebugRendering2);
  AddSubTest("Load Scene", SubTests::LoadScene);
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
}

WResult WGameEngineTestBasics::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  if (iIdentifier == SubTests::ManyMeshes)
  {
    m_pOwnApplication->SubTestManyMeshesSetup();
    return W_SUCCESS;
  }

  if (iIdentifier == SubTests::Skybox)
  {
    m_pOwnApplication->SubTestSkyboxSetup();
    return W_SUCCESS;
  }

  if (iIdentifier == SubTests::DebugRendering || iIdentifier == SubTests::DebugRendering2)
  {
    m_pOwnApplication->SubTestDebugRenderingSetup();
    return W_SUCCESS;
  }

  if (iIdentifier == SubTests::LoadScene)
  {
    m_pOwnApplication->SubTestLoadSceneSetup();
    return W_SUCCESS;
  }

  if (iIdentifier == SubTests::GameObjectReferences)
  {
    m_pOwnApplication->SubTestGoReferenceSetup();
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WTestAppRun WGameEngineTestBasics::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ManyMeshes)
    return m_pOwnApplication->SubTestManyMeshesExec(m_iFrame);

  if (iIdentifier == SubTests::Skybox)
    return m_pOwnApplication->SubTestSkyboxExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering)
    return m_pOwnApplication->SubTestDebugRenderingExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering2)
    return m_pOwnApplication->SubTestDebugRenderingExec2(m_iFrame);

  if (iIdentifier == SubTests::LoadScene)
    return m_pOwnApplication->SubTestLoadSceneExec(m_iFrame);

  if (iIdentifier == SubTests::GameObjectReferences)
    return m_pOwnApplication->SubTestGoReferenceExec(m_iFrame);

  W_ASSERT_NOT_IMPLEMENTED;
  return WTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

WGameEngineTestApplication_Basics::WGameEngineTestApplication_Basics()
  : WGameEngineTestApplication("Basics")
{
}

void WGameEngineTestApplication_Basics::SubTestManyMeshesSetup()
{
  W_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  WMeshResourceHandle hMesh = WResourceManager::LoadResource<WMeshResource>("Meshes/MissingMesh.WBinMesh");
  WMaterialResourceHandle hMaterial = WResourceManager::LoadResource<WMaterialResource>("{ b93a6e80-5bf7-48f0-b7b3-2097d0133159 }"); // FullbrightWhite
  WInt32 dim = 15;

  for (WInt32 z = -dim; z <= dim; ++z)
  {
    for (WInt32 y = -dim; y <= dim; ++y)
    {
      for (WInt32 x = -dim; x <= dim; ++x)
      {
        WGameObjectDesc go;
        go.m_LocalPosition.Set(x * 5.0f, y * 5.0f, z * 5.0f);

        WGameObject* pObject;
        m_pWorld->CreateObject(go, pObject);

        WMeshComponent* pMesh;
        m_pWorld->GetOrCreateComponentManager<WMeshComponentManager>()->CreateComponent(pObject, pMesh);
        pMesh->SetMaterial(0, hMaterial);
        pMesh->SetMesh(hMesh);
      }
    }
  }
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestManyMeshesExec(WInt32 iCurFrame)
{
  {
    auto pCamera = WDynamicCast<WGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(WCameraMode::PerspectiveFixedFovY, 100.0f, 1.0f, 1000.0f);
    WVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + WVec3(1, 0, 0), WVec3(0, 0, 1));
  }

  WResourceManager::ForceNoFallbackAcquisition(3);

  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  if (iCurFrame > 3)
  {
    W_TEST_IMAGE(0, 150);

    return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////


void WGameEngineTestApplication_Basics::SubTestSkyboxSetup()
{
  W_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  WTextureCubeResourceHandle hSkybox = WResourceManager::LoadResource<WTextureCubeResource>("Textures/Cubemap/WLogo_Cube_DXT1_Mips_D.dds");
  WMeshResourceHandle hMesh = WResourceManager::LoadResource<WMeshResource>("Meshes/MissingMesh.WBinMesh");
  WMaterialResourceHandle hMaterial = WResourceManager::LoadResource<WMaterialResource>("{ b93a6e80-5bf7-48f0-b7b3-2097d0133159 }"); // FullbrightWhite
  // Skybox
  {
    WGameObjectDesc go;
    go.m_LocalPosition.SetZero();

    WGameObject* pObject;
    m_pWorld->CreateObject(go, pObject);

    WSkyBoxComponent* pSkybox;
    m_pWorld->GetOrCreateComponentManager<WSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);

    pSkybox->SetCubeMap(hSkybox);
  }

  // some foreground objects
  {
    WInt32 dim = 5;

    for (WInt32 z = -dim; z <= dim; ++z)
    {
      for (WInt32 y = -dim; y <= dim; ++y)
      {
        for (WInt32 x = -dim; x <= dim; ++x)
        {
          WGameObjectDesc go;
          go.m_LocalPosition.Set(x * 10.0f, y * 10.0f, z * 10.0f);

          WGameObject* pObject;
          m_pWorld->CreateObject(go, pObject);

          WMeshComponent* pMesh;
          m_pWorld->GetOrCreateComponentManager<WMeshComponentManager>()->CreateComponent(pObject, pMesh);
          pMesh->SetMaterial(0, hMaterial);
          pMesh->SetMesh(hMesh);
        }
      }
    }
  }
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestSkyboxExec(WInt32 iCurFrame)
{
  WResourceManager::ForceNoFallbackAcquisition(3);

  auto pCamera = WDynamicCast<WGameState*>(GetActiveGameState())->GetMainCamera();
  pCamera->SetCameraMode(WCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
  WVec3 pos = WVec3(iCurFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + WVec3(1, 0, 0), WVec3(0, 0, 1));
  pCamera->RotateGlobally(WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(0), WAngle::MakeFromDegree(iCurFrame * 80.0f));

  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  if (iCurFrame < 5)
    return WTestAppRun::Continue;

  W_TEST_IMAGE(iCurFrame - 5, 150);

  if (iCurFrame < 8)
    return WTestAppRun::Continue;

  return WTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void WGameEngineTestApplication_Basics::SubTestDebugRenderingSetup()
{
  W_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  WRenderWorld::ResetFrameCounter();
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestDebugRenderingExec(WInt32 iCurFrame)
{
  {
    auto pCamera = WDynamicCast<WGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(WCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    WVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + WVec3(1, 0, 0), WVec3(0, 0, 1));
  }

  // line box
  {
    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3(10, -5, 1), WVec3(1, 2, 3));

    WTransform t;
    t.SetIdentity();
    t.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(25));
    WDebugRenderer::DrawLineBox(m_pWorld.Borrow(), bbox, WColor::HotPink, t);
  }

  // line box
  {
    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3(10, -3, 1), WVec3(1, 2, 3));

    WTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(25));
    WDebugRenderer::DrawLineBoxCorners(m_pWorld.Borrow(), bbox, 0.5f, WColor::DeepPink, t);
  }

  // 2D Rect
  {
    WDebugRenderer::Draw2DRectangle(m_pWorld.Borrow(), WRectFloat(10, 50, 35, 15), 0.1f, WColor::LawnGreen);
  }

  // Sphere
  {
    WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(WVec3(8, -5, -4), 2);
    WDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), sphere, WColor::Tomato);
  }

  // Solid box
  {
    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3(10, -5, 1), WVec3(1, 2, 3));

    WDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, WColor::BurlyWood);
  }

  // Text
  {
    WDebugRenderer::Draw2DText(m_pWorld.Borrow(), "Not 'a test\"", WVec2I32(30, 10), WColor::AntiqueWhite, 24);
    WDebugRenderer::Draw2DText(m_pWorld.Borrow(), "!@#$%^&*()_[]{}|", WVec2I32(20, 200), WColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    WFrustum f = WFrustum::MakeFromFOV(WVec3(5, 7, 3), WVec3(0, -1, 0), WVec3(0, 0, 1), WAngle::MakeFromDegree(30), WAngle::MakeFromDegree(20), 0.1f, 5.0f);
    WDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, WColor::Cornsilk);
  }

  // Lines
  {
    WTempHybridArray<WDebugRendererLine, 4> lines;
    lines.PushBack(WDebugRendererLine(WVec3(3, -4, -4), WVec3(4, -2, -3)));
    lines.PushBack(WDebugRendererLine(WVec3(4, -2, -3), WVec3(2, 2, -2)));
    WDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, WColor::SkyBlue);
  }

  // Triangles
  {
    WTempHybridArray<WDebugRendererTriangle, 4> tris;
    tris.PushBack(WDebugRendererTriangle(WVec3(7, 0, 0), WVec3(7, 2, 0), WVec3(7, 2, 1)));
    tris.PushBack(WDebugRendererTriangle(WVec3(7, 3, 0), WVec3(7, 1, 0), WVec3(7, 3, 1)));
    WDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), tris, WColor::Gainsboro);
  }

  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return WTestAppRun::Continue;

  WStringView sRendererName = WGALDevice::GetDefaultDevice()->GetRenderer();
  const bool bRandomlyChangesLineThicknessOnDriverUpdate = sRendererName.IsEqual_NoCase("DX11") && WGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia");

  W_TEST_LINE_IMAGE(0, bRandomlyChangesLineThicknessOnDriverUpdate ? 700 : 150);

  return WTestAppRun::Quit;
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestDebugRenderingExec2(WInt32 iCurFrame)
{
  {
    auto pCamera = WDynamicCast<WGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(WCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    WVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + WVec3(1, 0, 0), WVec3(0, 0, 1));
  }

  // Text
  {
    WDebugRenderer::Draw2DText(m_pWorld.Borrow(), WFmt("Frame# {}", WRenderWorld::GetFrameCounter()), WVec2I32(10, 10), WColor::AntiqueWhite, 24);
    WDebugRenderer::DrawInfoText(m_pWorld.Borrow(), WDebugTextPlacement::BottomLeft, "test", WFmt("Frame# {}", WRenderWorld::GetFrameCounter()));
    WDebugRenderer::DrawInfoText(m_pWorld.Borrow(), WDebugTextPlacement::BottomRight, "test", "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|");
  }

  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return WTestAppRun::Continue;

  W_TEST_IMAGE(0, 150);

  return WTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void WGameEngineTestApplication_Basics::SubTestLoadSceneSetup()
{
  WResourceManager::ForceNoFallbackAcquisition(3);
  WRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/Lighting.WBinScene").IgnoreResult();
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestLoadSceneExec(WInt32 iCurFrame)
{
  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 1:
      W_TEST_IMAGE(0, 150);
      break;

    case 2:
      W_TEST_IMAGE(1, 150);
      return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}

void WGameEngineTestApplication_Basics::SubTestGoReferenceSetup()
{
  WResourceManager::ForceNoFallbackAcquisition(3);
  WRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/GoReferences.WBinScene").IgnoreResult();
}

WTestAppRun WGameEngineTestApplication_Basics::SubTestGoReferenceExec(WInt32 iCurFrame)
{
  Run();
  if (ShouldApplicationQuit())
    return WTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 5:
      W_TEST_IMAGE(0, 100);
      return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}
