#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/ImageConversion.h>

// Constant buffer definition is shared between shader code and C++
#include <RendererCore/../../../Data/Samples/TextureSample/Shaders/SampleConstantBuffer.h>

class CustomTextureResourceLoader : public WTextureResourceLoader
{
public:
  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
};

const WInt32 g_iMaxHalfExtent = 20;
const bool g_bForceImmediateLoading = false;
const bool g_bPreloadAllTextures = false;

class TextureSample : public WApplication
{
  CustomTextureResourceLoader m_TextureResourceLoader;
  WConstantBufferStorageHandle m_hSampleConstants;
  WConstantBufferStorage<WTextureSampleConstants>* m_pSampleConstantBuffer;

public:
  using SUPER = WApplication;

  TextureSample()
    : WApplication("Texture Sample")
  {
    m_vCameraPosition.SetZero();
  }

  void AfterCoreSystemsStartup() override
  {
    WStringBuilder sProjectDir = ">sdk/Data/Samples/TextureSample";
    WStringBuilder sProjectDirResolved;
    WFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();

    WFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    // setup the 'asset management system'
    {
      // which redirection table to search
      WDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.WAsset";
      // which platform assets to use
      WDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/Default/";
    }

    WFileSystem::AddDataDirectory("", "", ":", WDataDirUsage::AllowWrites).IgnoreResult();
    WFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", WDataDirUsage::AllowWrites).IgnoreResult();                                  // writing to the binary directory
    WFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", WDataDirUsage::AllowWrites).IgnoreResult();                 // for shader files
    WFileSystem::AddDataDirectory(">user/WorldEngine Project/TextureSample", "AppData", "appdata", WDataDirUsage::AllowWrites).IgnoreResult(); // app user data
    WFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").IgnoreResult();
    WFileSystem::AddDataDirectory(">project/", "Project", "project", WDataDirUsage::AllowWrites).IgnoreResult();

    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);

    WTelemetry::CreateServer();
    WPlugin::LoadPlugin("WInspectorPlugin", WPluginLoadFlags::PluginIsOptional).IgnoreResult();


#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultRenderer = "Vulkan";
#else
    constexpr const char* szDefaultRenderer = "DX11";
#endif

    WStringView sRendererName = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
    const char* szShaderModel = "";
    const char* szShaderCompiler = "";
    WGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

    WShaderManager::Configure(szShaderModel, true);
    W_VERIFY(WPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

    // Register Input
    {
      WInputActionConfig cfg;

      cfg = WInputManager::GetInputActionConfig("Main", "CloseApp");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyEscape;
      WInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

      cfg = WInputManager::GetInputActionConfig("Main", "MovePosX");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosX;
      cfg.m_bApplyTimeScaling = false;
      WInputManager::SetInputActionConfig("Main", "MovePosX", cfg, true);

      cfg = WInputManager::GetInputActionConfig("Main", "MoveNegX");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegX;
      cfg.m_bApplyTimeScaling = false;
      WInputManager::SetInputActionConfig("Main", "MoveNegX", cfg, true);

      cfg = WInputManager::GetInputActionConfig("Main", "MovePosY");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMovePosY;
      cfg.m_bApplyTimeScaling = false;
      WInputManager::SetInputActionConfig("Main", "MovePosY", cfg, true);

      cfg = WInputManager::GetInputActionConfig("Main", "MoveNegY");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseMoveNegY;
      cfg.m_bApplyTimeScaling = false;
      WInputManager::SetInputActionConfig("Main", "MoveNegY", cfg, true);

      cfg = WInputManager::GetInputActionConfig("Main", "MouseDown");
      cfg.m_sInputSlotTrigger[0] = WInputSlot_MouseButton0;
      cfg.m_bApplyTimeScaling = false;
      WInputManager::SetInputActionConfig("Main", "MouseDown", cfg, true);
    }

    // Create a window for rendering
    {
      WWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width = 1024;
      WindowCreationDesc.m_Resolution.height = 768;
      m_pWindow = W_DEFAULT_NEW(WWindow);
      m_pWindow->Initialize(WindowCreationDesc).IgnoreResult();

      m_pWindow->WindowEvents().AddEventHandler([this](const WWindowEvent& e)
        {
          if (e.m_Type == WWindowEvent::Type::CloseButtonClicked)
          {
            this->QuitApplication();
          }
          //
        });
    }

    if (auto pInput = WInputManager::GetInputDeviceOfType<WInputDeviceMouseKeyboard>())
    {
      pInput->SetClipMouseCursor(WMouseCursorClipMode::NoClip);
      pInput->SetShowMouseCursor(true);
    }

    // Create a device
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

      WGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = m_pWindow->GetClientAreaSize().width;
      texDesc.m_uiHeight = m_pWindow->GetClientAreaSize().height;
      texDesc.m_Format = WGALResourceFormat::D24S8;
      texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);

      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);

      m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);
    }

    // Setup Shaders and Materials
    {
      // the shader (referenced by the material) also defines the render pipeline state, such as backface-culling and depth-testing

      m_hMaterial = WResourceManager::LoadResource<WMaterialResource>("Materials/Texture.WMaterial");

      // Create the mesh that we use for rendering
      CreateSquareMesh();
    }

    // Setup default resources
    {
      WTexture2DResourceHandle hFallback = WResourceManager::LoadResource<WTexture2DResource>("Textures/Reference_D.dds");
      WTexture2DResourceHandle hMissing = WResourceManager::LoadResource<WTexture2DResource>("Textures/MissingResource_D.dds");

      WResourceManager::SetResourceTypeLoadingFallback<WTexture2DResource>(hFallback);
      WResourceManager::SetResourceTypeMissingFallback<WTexture2DResource>(hMissing);

      // redirect all texture load operations through our custom loader, so that we can duplicate the single source texture
      // that we have as often as we like (to waste memory)
      WResourceManager::SetResourceTypeLoader<WTexture2DResource>(&m_TextureResourceLoader);
    }

    // Setup constant buffer that this sample uses
    {
      m_hSampleConstants = WRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
    }

    // Pre-allocate all textures
    {
      // we only do this to be able to see the unloaded resources in the WInspector
      // this does NOT preload the resources

      WStringBuilder sResourceName;
      for (WInt32 y = -g_iMaxHalfExtent; y < g_iMaxHalfExtent; ++y)
      {
        for (WInt32 x = -g_iMaxHalfExtent; x < g_iMaxHalfExtent; ++x)
        {
          sResourceName.SetPrintf("Loaded_%+03i_%+03i_D", x, y);

          WTexture2DResourceHandle hTexture = WResourceManager::LoadResource<WTexture2DResource>(sResourceName);

          if (g_bPreloadAllTextures)
            WResourceManager::PreloadResource(hTexture);
        }
      }
    }
  }


  void Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (WInputManager::GetInputActionState("Main", "CloseApp") == WKeyState::Pressed)
    {
      QuitApplication();
      return;
    }

    // make sure time goes on
    WClock::GetGlobalClock()->Update();

    if (WInputManager::GetInputActionState("Main", "MouseDown") == WKeyState::Down)
    {
      float fInputValue = 0.0f;
      const float fMouseSpeed = 20.0f;

      if (WInputManager::GetInputActionState("Main", "MovePosX", &fInputValue) != WKeyState::Up)
        m_vCameraPosition.x -= fInputValue * fMouseSpeed;
      if (WInputManager::GetInputActionState("Main", "MoveNegX", &fInputValue) != WKeyState::Up)
        m_vCameraPosition.x += fInputValue * fMouseSpeed;
      if (WInputManager::GetInputActionState("Main", "MovePosY", &fInputValue) != WKeyState::Up)
        m_vCameraPosition.y += fInputValue * fMouseSpeed;
      if (WInputManager::GetInputActionState("Main", "MoveNegY", &fInputValue) != WKeyState::Up)
        m_vCameraPosition.y -= fInputValue * fMouseSpeed;
    }

    // update all input state
    WInputManager::Update(WClock::GetGlobalClock()->GetTimeDiff());

    // make sure telemetry is sent out regularly
    WTelemetry::PerFrameUpdate();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
      m_pDevice->BeginFrame();

      WGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("WTextureSampleMainPass");

      WGALRenderingSetup renderingSetup;
      const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
      renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture())).SetDepthStencilTarget(m_hBBDSV);
      renderingSetup.SetClearColor(0).SetClearDepth();

      const float fWindowWidth = (float)m_pWindow->GetClientAreaSize().width;
      const float fWindowHeight = (float)m_pWindow->GetClientAreaSize().height;

      WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, WRectFloat(0.0f, 0.0f, fWindowWidth, fWindowHeight));

      WMat4 Proj = WGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -fWindowWidth * 0.5f, m_vCameraPosition.x + fWindowWidth * 0.5f, m_vCameraPosition.y + -fWindowHeight * 0.5f, m_vCameraPosition.y + fWindowHeight * 0.5f, -1.0f, 1.0f);

      WBindGroupBuilder& bindGroupSample = WRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroupSample.BindBuffer("WTextureSampleConstants", m_hSampleConstants);
      WRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);

      WMat4 mTransform = WMat4::MakeIdentity();

      WInt32 iLeftBound = (WInt32)WMath::Floor((m_vCameraPosition.x - fWindowWidth * 0.5f) / 100.0f);
      WInt32 iLowerBound = (WInt32)WMath::Floor((m_vCameraPosition.y - fWindowHeight * 0.5f) / 100.0f);
      WInt32 iRightBound = (WInt32)WMath::Ceil((m_vCameraPosition.x + fWindowWidth * 0.5f) / 100.0f) + 1;
      WInt32 iUpperBound = (WInt32)WMath::Ceil((m_vCameraPosition.y + fWindowHeight * 0.5f) / 100.0f) + 1;

      iLeftBound = WMath::Max(iLeftBound, -g_iMaxHalfExtent);
      iRightBound = WMath::Min(iRightBound, g_iMaxHalfExtent);
      iLowerBound = WMath::Max(iLowerBound, -g_iMaxHalfExtent);
      iUpperBound = WMath::Min(iUpperBound, g_iMaxHalfExtent);

      WStringBuilder sResourceName;

      for (WInt32 y = iLowerBound; y < iUpperBound; ++y)
      {
        for (WInt32 x = iLeftBound; x < iRightBound; ++x)
        {
          mTransform.SetTranslationVector(WVec3((float)x * 100.0f, (float)y * 100.0f, 0));

          // Update the constant buffer
          {
            WTextureSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();
            cb.ModelMatrix = mTransform;
            cb.ViewProjectionMatrix = Proj;
          }

          sResourceName.SetPrintf("Loaded_%+03i_%+03i_D", x, y);

          WTexture2DResourceHandle hTexture = WResourceManager::LoadResource<WTexture2DResource>(sResourceName);

          // force immediate loading
          if (g_bForceImmediateLoading)
            WResourceLock<WTexture2DResource> l(hTexture, WResourceAcquireMode::BlockTillLoaded);

          bindGroupSample.BindTexture("DiffuseTexture", hTexture);
          WRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMeshBuffer);
          WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
        }
      }

      WRenderContext::GetDefaultInstance()->EndRendering();
      m_pDevice->EndCommands(pCommandEncoder);

      m_pDevice->EndFrame();
    }

    // needs to be called once per frame
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

    m_hMaterial.Invalidate();
    m_hQuadMeshBuffer.Invalidate();

    // tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    WStartup::ShutdownHighLevelSystems();

    WResourceManager::FreeAllUnusedResources();

    m_pDevice->DestroySwapChain(m_hSwapChain);

    // now we can destroy the graphics device
    m_pDevice->Shutdown().IgnoreResult();

    W_DEFAULT_DELETE(m_pDevice);

    // finally destroy the window
    m_pWindow->DestroyWindow();
    W_DEFAULT_DELETE(m_pWindow);
  }

  void CreateSquareMesh()
  {
    struct Vertex
    {
      WVec3 Position;
      WVec2 TexCoord0;
    };

    WGeometry geom;
    WGeometry::GeoOptions opt;
    opt.m_Color = WColor::Black;
    geom.AddRect(WVec2(100, 100), 1, 1, opt);

    WDynamicArray<Vertex> Vertices;
    WDynamicArray<WUInt16> Indices;

    Vertices.Reserve(geom.GetVertices().GetCount());
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    WMeshBufferResourceDescriptor desc;
    desc.AddCommonStreams(true);

    desc.AllocateStreams(geom.GetVertices().GetCount(), WGALPrimitiveTopology::Triangles, geom.GetPolygons().GetCount() * 2);

    for (WUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      WVec2 tc(geom.GetVertices()[v].m_vPosition.x / 100.0f, geom.GetVertices()[v].m_vPosition.y / -100.0f);
      tc += WVec2(0.5f);

      desc.SetPosition(v, geom.GetVertices()[v].m_vPosition);
      desc.SetTexCoord0(v, tc);
    }

    WUInt32 t = 0;
    for (WUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (WUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        desc.SetTriangleIndices(t, geom.GetPolygons()[p].m_Vertices[0], geom.GetPolygons()[p].m_Vertices[v + 1], geom.GetPolygons()[p].m_Vertices[v + 2]);

        ++t;
      }
    }

    m_hQuadMeshBuffer = WResourceManager::GetExistingResource<WMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}");

    if (!m_hQuadMeshBuffer.IsValid())
      m_hQuadMeshBuffer = WResourceManager::GetOrCreateResource<WMeshBufferResource>("{E692442B-9E15-46C5-8A00-1B07C02BF8F7}", std::move(desc));
  }

private:
  WWindow* m_pWindow;
  WGALDevice* m_pDevice;

  WGALSwapChainHandle m_hSwapChain;
  WGALRenderTargetViewHandle m_hBBDSV;
  WGALTextureHandle m_hDepthStencilTexture;

  WMaterialResourceHandle m_hMaterial;
  WMeshBufferResourceHandle m_hQuadMeshBuffer;

  WVec2 m_vCameraPosition;
};

WResourceLoadData CustomTextureResourceLoader::OpenDataStream(const WResource* pResource)
{
  WString sFileToLoad = pResource->GetResourceID();

  if (sFileToLoad.StartsWith("Loaded"))
  {
    sFileToLoad = "Textures/Loaded_D.dds"; // redirect all "Loaded_XYZ" files to the same source file
  }

  // the entire rest is copied from WTextureResourceLoader

  LoadedData* pData = W_DEFAULT_NEW(LoadedData);

  WResourceLoadData res;

#if W_ENABLED(W_SUPPORTS_FILE_STATS)
  {
    WFileReader File;
    if (File.Open(sFileToLoad).Failed())
      return res;

    WFileStats stat;
    if (WOSFile::GetFileStats(File.GetFilePathAbsolute(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
  }
#endif


  if (pData->m_Image.LoadFrom(sFileToLoad).Failed())
    return res;

  if (pData->m_Image.GetImageFormat() == WImageFormat::B8G8R8_UNORM)
  {
    WImageConversion::Convert(pData->m_Image, pData->m_Image, WImageFormat::B8G8R8A8_UNORM).IgnoreResult();
  }

  WMemoryStreamWriter w(&pData->m_Storage);

  WImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(WImage*)).IgnoreResult();

  /// This is a hack to get the SRGB information for the texture

  const WStringBuilder sName = WPathUtils::GetFileName(sFileToLoad);

  bool bIsFallback = false;
  bool bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

  w << bIsFallback;
  w << bSRGB;

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

W_APPLICATION_ENTRY_POINT(TextureSample);
