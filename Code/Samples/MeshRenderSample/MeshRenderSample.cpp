#include <Core/Graphics/Geometry.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/DeviceFactory.h>

// Constant buffer definition is shared between shader code and C++
#include <RendererCore/../../../Data/Samples/MeshRenderSample/Shaders/SampleConstantBuffer.h>


#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
constexpr const char* szDefaultRenderer = "Vulkan";
#else
constexpr const char* szDefaultRenderer = "DX11";
#endif

/// The application class.
/// Instantiated and run through the W_APPLICATION_ENTRY_POINT macro at the end of this file.
class MeshRenderSample : public WApplication
{
public:
  MeshRenderSample()
    : WApplication("MeshRenderSample")
  {
  }

  WResult BeforeCoreSystemsStartup() override
  {
    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);

    return WApplication::BeforeCoreSystemsStartup();
  }

  void AfterCoreSystemsStartup() override
  {
    WDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/Default.WAidlt";
    WDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";
    WStringBuilder sProjectDir = ">sdk/Data/Samples/MeshRenderSample";
    WStringBuilder sProjectDirResolved;
    WFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).AssertSuccess();

    WFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    if (WFileSystem::AddDataDirectory(">sdk/Output", "ShaderCache", "shadercache", WDataDirUsage::AllowWrites).Failed())
    {
      WFileSystem::AddDataDirectory(">sdk/Output", "ShaderCache", "shadercache", WDataDirUsage::ReadOnly).AssertSuccess();
    }

    WFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    WFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

    WStringView sRendererName = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);

    const char* szShaderModel = "";
    const char* szShaderCompiler = "";
    WGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

    WShaderManager::Configure(szShaderModel, true);
    WPlugin::LoadPlugin(szShaderCompiler).IgnoreResult();

    // Create a window for rendering (this also sets up the mouse/keyboard input device)
    {
      WWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width = 1024;
      WindowCreationDesc.m_Resolution.height = 768;

      m_pWindow = W_DEFAULT_NEW(WWindow);
      m_pWindow->Initialize(WindowCreationDesc).AssertSuccess();

      m_pWindow->WindowEvents().AddEventHandler([this](const WWindowEvent& e)
        {
          if (e.m_Type == WWindowEvent::Type::CloseButtonClicked)
          {
            this->QuitApplication();
          }
          //
        });

      if (auto pInput = WDynamicCast<WInputDeviceMouseKeyboard*>(m_pWindow->GetInputDevice()))
      {
        pInput->SetShowMouseCursor(true);
        pInput->SetClipMouseCursor(WMouseCursorClipMode::NoClip);
      }
    }

    // Register Input
    {
      WInputActionConfig cfg;

      cfg = WInputManager::GetInputActionConfig("Main", "CloseApp");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyEscape;
      WInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
    }

    // Create a rendering device
    {
      WGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_bDebugDevice = true;

      m_pDevice = WGALDeviceFactory::CreateDevice(sRendererName, WFoundation::GetDefaultAllocator(), DeviceInit);
      W_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sRendererName);
      W_VERIFY(m_pDevice->Init() == W_SUCCESS, "Device init failed!");

      WGALDevice::SetDefaultDevice(m_pDevice);
    }

    // now that we have a window and device, tell the engine to initialize the rendering infrastructure
    WStartup::StartupHighLevelSystems();

    // Create a Swapchain
    {
      WGALWindowSwapChainCreationDescription swapChainDesc;
      swapChainDesc.m_pWindow = m_pWindow;
      swapChainDesc.m_SampleCount = WGALMSAASampleCount::None;
      m_hSwapChain = WGALWindowSwapChain::Create(swapChainDesc);

      const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
      const WSizeU32 windowSize = m_pWindow->GetClientAreaSize();

      WGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = windowSize.width;
      texDesc.m_uiHeight = windowSize.height;
      texDesc.m_Format = WGALResourceFormat::D24S8;
      texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);

      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);

      m_hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
      m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);
    }

    // Setup Shaders and Materials
    {
      // the shader (referenced by the material) also defines the render pipeline state
      // such as backface-culling and depth-testing
      m_hMaterial1 = WResourceManager::LoadResource<WMaterialResource>("Materials/Sample1.WMaterial");
      m_hMaterial2 = WResourceManager::LoadResource<WMaterialResource>("Materials/Sample2.WMaterial");

      // Create the mesh that we use for rendering
      m_hMeshBuffer1 = CreateTorusMesh();
      m_hMeshBuffer2 = CreateSphereMesh();
    }

    // Setup constant buffer that this sample uses
    {
      m_hSampleConstants = WRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
    }
  }


  void Run() override
  {
    W_LOG_BLOCK("Frame");

    // make sure time goes on
    WClock::GetGlobalClock()->Update();

    // tick the input system
    WInputManager::Update(WClock::GetGlobalClock()->GetTimeDiff());

    // close if the Escape key was pressed
    if (WInputManager::GetInputActionState("Main", "CloseApp") == WKeyState::Pressed)
    {
      QuitApplication();
      return;
    }

    m_pWindow->ProcessWindowMessages();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
      m_pDevice->BeginFrame();

      WGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("WMeshRenderSampleMainPass");

      const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

      m_hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
      m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);

      WGALRenderingSetup renderingSetup;
      renderingSetup.SetColorTarget(0, m_hBBRTV).SetDepthStencilTarget(m_hBBDSV);
      renderingSetup.SetClearColor(0).SetClearDepth();

      const float fWindowWidth = (float)m_pWindow->GetClientAreaSize().width;
      const float fWindowHeight = (float)m_pWindow->GetClientAreaSize().height;

      WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, WRectFloat(0.0f, 0.0f, fWindowWidth, fWindowHeight));

      const WMat4 mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(WAngle::MakeFromDegree(70), fWindowWidth / fWindowHeight, 0.1f, 1000.0f);

      WBindGroupBuilder& bindGroupSample = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupSample.BindBuffer("WMeshRenderSampleConstants", m_hSampleConstants);

      m_CameraRotation += WAngle::MakeFromDegree(WClock::GetGlobalClock()->GetTimeDiff().AsFloatInSeconds() * 90.0f);
      m_CameraRotation.NormalizeRange();

      WVec3 vCamPos(0);
      vCamPos.x = WMath::Sin(m_CameraRotation) * 5.0f;
      vCamPos.y = WMath::Cos(m_CameraRotation) * 5.0f;
      vCamPos.z = 3.0f;

      const WMat4 mView = WGraphicsUtils::CreateLookAtViewMatrix(vCamPos, WVec3::MakeZero(), WVec3(0, 0, 1));

      // Update the constant buffer
      {
        WMeshRenderSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();
        cb.ModelMatrix = mView;
        cb.ViewProjectionMatrix = mProj;
        cb.TintColor = WColor::CornflowerBlue;
      }

      WRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial1);
      WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hMeshBuffer1);
      WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();

      WRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial2);
      WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hMeshBuffer2);
      WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();

      WRenderContext::GetDefaultInstance()->EndRendering();
      m_pDevice->EndCommands(pCommandEncoder);

      m_pDevice->EndFrame();
    }

    // needs to be called once per frame to finish resource loading
    WResourceManager::PerFrameUpdate();

    // tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    WTaskSystem::FinishFrameTasks();
  }

  void BeforeCoreSystemsShutdown() override
  {
    // make sure that no textures are continue to be streamed in while the engine shuts down
    WResourceManager::EngineAboutToShutdown();

    WRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);

    m_hMaterial1.Invalidate();
    m_hMaterial2.Invalidate();
    m_hMeshBuffer1.Invalidate();
    m_hMeshBuffer2.Invalidate();

    // tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    WStartup::ShutdownHighLevelSystems();

    WResourceManager::FreeAllUnusedResources();

    m_pDevice->DestroySwapChain(m_hSwapChain);

    // now we can destroy the graphics device
    m_pDevice->Shutdown().AssertSuccess();
    W_DEFAULT_DELETE(m_pDevice);

    // finally destroy the window
    m_pWindow->DestroyWindow();
    W_DEFAULT_DELETE(m_pWindow);
  }

  WMeshBufferResourceHandle CreateTorusMesh()
  {
    WGeometry geom;
    WGeometry::GeoOptions opt;
    opt.m_Color = WColor::Black;
    geom.AddTorus(2.0f, 3.5f, 24, 12, true, opt);
    geom.TriangulatePolygons();

    WMeshBufferResourceDescriptor desc;
    desc.AddCommonStreams();

    desc.AllocateStreamsFromGeometry(geom);

    return WResourceManager::GetOrCreateResource<WMeshBufferResource>("TorusMesh", std::move(desc));
  }

  WMeshBufferResourceHandle CreateSphereMesh()
  {
    WGeometry geom;
    WGeometry::GeoOptions opt;
    opt.m_Color = WColor::Black;
    geom.AddGeodesicSphere(1.5f, 1);
    geom.TriangulatePolygons();

    WMeshBufferResourceDescriptor desc;
    desc.AddCommonStreams();

    desc.AllocateStreamsFromGeometry(geom);

    return WResourceManager::GetOrCreateResource<WMeshBufferResource>("SphereMesh", std::move(desc));
  }

private:
  WWindow* m_pWindow = nullptr;
  WGALDevice* m_pDevice = nullptr;

  WGALSwapChainHandle m_hSwapChain;
  WGALRenderTargetViewHandle m_hBBRTV;
  WGALRenderTargetViewHandle m_hBBDSV;
  WGALTextureHandle m_hDepthStencilTexture;

  WConstantBufferStorageHandle m_hSampleConstants;
  WConstantBufferStorage<WMeshRenderSampleConstants>* m_pSampleConstantBuffer;
  WMaterialResourceHandle m_hMaterial1;
  WMaterialResourceHandle m_hMaterial2;
  WMeshBufferResourceHandle m_hMeshBuffer1;
  WMeshBufferResourceHandle m_hMeshBuffer2;
  WAngle m_CameraRotation = WAngle::MakeZero();
};

W_APPLICATION_ENTRY_POINT(MeshRenderSample);
