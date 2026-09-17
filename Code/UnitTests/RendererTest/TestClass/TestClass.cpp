#include <RendererTest/RendererTestPCH.h>

#include "TestClass.h"

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Utils/ResourceStateTracker.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

WGraphicsTest::WGraphicsTest() = default;

WResult WGraphicsTest::InitializeTest()
{
  return W_SUCCESS;
}

WResult WGraphicsTest::DeInitializeTest()
{
  return W_SUCCESS;
}

WResult WGraphicsTest::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  // initialize everything up to 'core'
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;
  return W_SUCCESS;
}

WResult WGraphicsTest::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_Readback.Reset();
  ShutdownRenderer();
  // shut down completely
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();
  return W_SUCCESS;
}

WSizeU32 WGraphicsTest::GetResolution() const
{
  return m_pWindow->GetClientAreaSize();
}


WResult WGraphicsTest::CreateRenderer(WGALDevice*& out_pDevice)
{
  {
    WFileSystem::SetSpecialDirectory("testout", WTestFramework::GetInstance()->GetAbsOutputPath());

    WStringBuilder sBaseDir = ">sdk/Data/Base/";
    WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    W_SUCCEED_OR_RETURN(WFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", WDataDirUsage::AllowWrites)); // for shader files

    W_SUCCEED_OR_RETURN(WFileSystem::AddDataDirectory(sBaseDir, "Base"));

    W_SUCCEED_OR_RETURN(WFileSystem::AddDataDirectory(">Wtest/", "ImageComparisonDataDir", "imgout", WDataDirUsage::AllowWrites));

    W_SUCCEED_OR_RETURN(WFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

    sReadDir.Set(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
    W_SUCCEED_OR_RETURN(WFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  }

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
  if (WPlugin::LoadPlugin(szShaderCompiler).Failed())
    WLog::Warning("Shader compiler '{}' plugin not found", szShaderCompiler);

  // Create a device
  {
    WGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = WCommandLineUtils::GetGlobalInstance()->GetBoolOption("-debugdevice", false);
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    DeviceInit.m_bDebugDevice = true;
#endif
    out_pDevice = WGALDeviceFactory::CreateDevice(sRendererName, WFoundation::GetDefaultAllocator(), DeviceInit);
    if (out_pDevice->Init().Failed())
      return W_FAILURE;

    WGALDevice::SetDefaultDevice(out_pDevice);
  }

  WTestFramework::GetInstance()->SetImageReferenceTagsFromEnvironment(W_PLATFORM_NAME, sRendererName, out_pDevice->GetCapabilities().m_sAdapterName);
  return W_SUCCESS;
}

const WGALDeviceCapabilities& WGraphicsTest::GetDeviceCapabilities()
{
  static WGALDeviceCapabilities* pCaps = nullptr;
  if (pCaps == nullptr)
  {
    pCaps = W_NEW(WStaticsAllocatorWrapper::GetAllocator(), WGALDeviceCapabilities);
    WStartup::StartupCoreSystems();
    SetupRenderer().AssertSuccess();
    const WGALDeviceCapabilities& caps = WGALDevice::GetDefaultDevice()->GetCapabilities();
    *pCaps = caps;
    ShutdownRenderer();
    WStartup::ShutdownCoreSystems();
  }
  return *pCaps;
}

WResult WGraphicsTest::SetupRenderer()
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::CreateRenderer(m_pDevice));

  m_hObjectTransformCB = WRenderContext::CreateConstantBufferStorage<ObjectCB>();
  m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Default.WShader");

  {
    // Unit cube mesh
    WGeometry geom;
    geom.AddBox(WVec3(1.0f), true);

    WGALPrimitiveTopology::Enum Topology = WGALPrimitiveTopology::Triangles;
    WMeshBufferResourceDescriptor desc;
    desc.AddCommonStreams();
    desc.AllocateStreamsFromGeometry(geom, Topology);

    m_hCubeUV = WResourceManager::GetOrCreateResource<WMeshBufferResource>("Texture2DBox", std::move(desc), "Texture2DBox");
  }

  WStartup::StartupHighLevelSystems();
  return W_SUCCESS;
}

void WGraphicsTest::ShutdownRenderer()
{
  m_Readback.Reset();
  W_ASSERT_DEV(m_pWindow == nullptr, "DestroyWindow needs to be called before ShutdownRenderer");
  m_hShader.Invalidate();
  m_hCubeUV.Invalidate();

  WRenderContext::DeleteConstantBufferStorage(m_hObjectTransformCB);

  WStartup::ShutdownHighLevelSystems();

  WResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    W_DEFAULT_DELETE(m_pDevice);
  }

  WFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

WResult WGraphicsTest::CreateWindow(WUInt32 uiResolutionX, WUInt32 uiResolutionY)
{
  // Create a window for rendering
  {
    WWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_bSetForegroundOnInit = false;

    m_pWindow = W_DEFAULT_NEW(WWindow);
    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return W_FAILURE;
  }

  // Create a Swapchain
  {
    WGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = WGALMSAASampleCount::None;
    m_hSwapChain = WGALWindowSwapChain::Create(swapChainDesc);
    if (m_hSwapChain.IsInvalidated())
    {
      return W_FAILURE;
    }
  }

  {
    WGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiResolutionX;
    texDesc.m_uiHeight = uiResolutionY;
    texDesc.m_Format = WGALResourceFormat::D24S8;
    texDesc.m_TextureFlags.Add(WGALTextureUsageFlags::RenderTarget);
    texDesc.m_ResourceAccess.m_bImmutable = false;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    if (m_hDepthStencilTexture.IsInvalidated())
    {
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

void WGraphicsTest::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }
    m_pDevice->WaitIdle();
  }


  if (m_pWindow)
  {
    m_pWindow->DestroyWindow();
    W_DEFAULT_DELETE(m_pWindow);
  }
}

void WGraphicsTest::BeginFrame()
{
  if (!m_hSwapChain.IsInvalidated())
    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
  m_pDevice->BeginFrame(m_iFrame);
}

void WGraphicsTest::EndFrame()
{
  if (m_pWindow)
  {
    m_pWindow->ProcessWindowMessages();
  }

  m_pDevice->EndFrame();

  WTaskSystem::FinishFrameTasks();
}


void WGraphicsTest::BeginCommands(const char* szPassName)
{
  W_ASSERT_DEV(m_pEncoder == nullptr, "Call EndCommands first before calling BeginCommands again");
  m_pEncoder = m_pDevice->BeginCommands(szPassName);
  m_pResourceStateTracker = W_DEFAULT_NEW(WGALResourceStateTracker, m_pDevice);
}


void WGraphicsTest::EndCommands()
{
  W_ASSERT_DEV(m_pEncoder != nullptr, "Call BeginCommands first before calling EndCommands");

  if (m_pResourceStateTracker != nullptr)
  {
    m_pResourceStateTracker->RevertTextureState(WMakeDelegate(&WGraphicsTest::TextureBarrier, this));
    m_pResourceStateTracker->RevertBufferState(WMakeDelegate(&WGraphicsTest::BufferBarrier, this));
    m_pResourceStateTracker = nullptr;
  }

  m_pDevice->EndCommands(m_pEncoder);
  m_pEncoder = nullptr;
}

WGALCommandEncoder* WGraphicsTest::BeginRendering(WColor clearColor, WUInt32 uiRenderTargetClearMask, WRectFloat* pViewport, WRectU32* pScissor)
{
  const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
  TransitionTexture(m_hDepthStencilTexture, WGALResourceState::DepthStencilWrite);

  WGALRenderingSetup renderingSetup;
  renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  if (uiRenderTargetClearMask & W_BIT(0))
  {
    renderingSetup.SetClearColor(0, clearColor);
  }
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.SetClearDepth().SetClearStencil();
  }
  WRectFloat viewport = WRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
  if (pViewport)
  {
    viewport = *pViewport;
  }

  WRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
  WRectU32 scissor = WRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
  if (pScissor)
  {
    scissor = *pScissor;
  }

  auto pCommandEncoder = WRenderContext::GetDefaultInstance()->GetCommandEncoder();
  pCommandEncoder->SetScissorRect(scissor);

  SetClipSpace();
  return pCommandEncoder;
}

void WGraphicsTest::TransitionTexture(WGALTextureHandle hTexture, WBitflags<WGALResourceState> newState, WGALTextureRange range, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEV(m_pResourceStateTracker != nullptr, "TransitionTexture can only be called between BeginCommands and EndCommands");
  m_pResourceStateTracker->ChangeState(hTexture, range, newState, stage, WMakeDelegate(&WGraphicsTest::TextureBarrier, this));
}

void WGraphicsTest::TransitionBuffer(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> newState, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEV(m_pResourceStateTracker != nullptr, "TransitionBuffer can only be called between BeginCommands and EndCommands");
  m_pResourceStateTracker->ChangeState(hBuffer, newState, stage, WMakeDelegate(&WGraphicsTest::BufferBarrier, this));
}

void WGraphicsTest::EndRendering()
{
  WRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
}

WGALResourceStateTracker* WGraphicsTest::GetResourceStateTracker()
{
  return m_pResourceStateTracker.Borrow();
}

void WGraphicsTest::SetClipSpace()
{
  static WHashedString sClipSpaceFlipped = WMakeHashedString("CLIP_SPACE_FLIPPED");
  static WHashedString sTrue = WMakeHashedString("TRUE");
  static WHashedString sFalse = WMakeHashedString("FALSE");
  WClipSpaceYMode::Enum clipSpace = WClipSpaceYMode::RenderToTextureDefault;
  WRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, clipSpace == WClipSpaceYMode::Flipped ? sTrue : sFalse);
}

void WGraphicsTest::RenderCube(WRectFloat viewport, WMat4 mMVP, WUInt32 uiRenderTargetClearMask, WGALTextureHandle hTexture, const WGALTextureRange& textureRange)
{
  WGALCommandEncoder* pCommandEncoder = BeginRendering(WColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);

  WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindTexture("DiffuseTexture", hTexture, textureRange);
  RenderObject(m_hCubeUV, mMVP, WColor(1, 1, 1, 1), WShaderBindFlags::None);
  EndRendering();
  if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
    W_TEST_IMAGE(m_iFrame, 100);
  }
};


WMat4 WGraphicsTest::CreateSimpleMVP(float fAspectRatio)
{
  WCamera cam;
  cam.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 0, 1));
  WMat4 mProj;
  cam.GetProjectionMatrix(fAspectRatio, mProj);
  WMat4 mView = cam.GetViewMatrix();

  WMat4 mTransform = WMat4::MakeTranslation(WVec3(0.0f, 0.0f, -1.2f));
  return mProj * mView * mTransform;
}

void WGraphicsTest::ReadbackImage(WRenderGraph& ref_graph)
{
  WGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  WRenderGraphTextureHandle hGraphTexture = ref_graph.ImportTexture(hBBTexture);

  auto pass = ref_graph.AddTransferPass("ReadbackTexture");
  pass.ReadTexture(hGraphTexture, {}, WGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([=](const WRenderGraphContext& context)
    {
      m_bReadBackInProgress = true;
      m_Readback.ReadbackTexture(*context.GetCommandEncoder(), context.ResolveTexture(hGraphTexture));
      context.GetCommandEncoder()->Flush(); //
    });
}

WResult WGraphicsTest::GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber)
{
  WGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  const WGALTexture* pBackbuffer = WGALDevice::GetDefaultDevice()->GetTexture(hBBTexture);

  if (!m_bReadBackInProgress)
  {
    auto pCommandEncoder = WRenderContext::GetDefaultInstance()->GetCommandEncoder();
    m_Readback.ReadbackTexture(*pCommandEncoder, hBBTexture);
    pCommandEncoder->Flush();
  }

  // Wait for results
  {
    WEnum<WGALAsyncResult> res = m_Readback.GetReadbackResult(WTime::MakeFromHours(1));
    W_ASSERT_ALWAYS(res == WGALAsyncResult::Ready, "Readback of texture failed");
  }

  WGALTextureSubresource sourceSubResource;
  WArrayPtr<WGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  WTempHybridArray<WGALSystemMemoryDescription, 1> memory;
  WReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock readback texture");
  WTextureUtils::CopySubResourceToImage(pBackbuffer->GetDescription(), sourceSubResource, memory[0], ref_img, true);

  m_bReadBackInProgress = false;
  return W_SUCCESS;
}

WMeshBufferResourceHandle WGraphicsTest::CreateMesh(const WGeometry& geom, const char* szResourceName)
{
  WMeshBufferResourceHandle hMesh;
  hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGALPrimitiveTopology::Enum Topology = WGALPrimitiveTopology::Triangles;
  if (geom.GetLines().GetCount() > 0)
    Topology = WGALPrimitiveTopology::Lines;

  WMeshBufferResourceDescriptor desc;
  desc.AddStream(WMeshVertexStreamType::Position);
  desc.AddStream(WMeshVertexStreamType::Color0);
  desc.AllocateStreamsFromGeometry(geom, Topology);

  hMesh = WResourceManager::GetOrCreateResource<WMeshBufferResource>(szResourceName, std::move(desc), szResourceName);

  return hMesh;
}

WMeshBufferResourceHandle WGraphicsTest::CreateSphere(WInt32 iSubDivs, float fRadius)
{
  WGeometry geom;
  geom.AddGeodesicSphere(fRadius, static_cast<WUInt8>(iSubDivs));

  WStringBuilder sName;
  sName.SetFormat("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

WMeshBufferResourceHandle WGraphicsTest::CreateTorus(WInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  WGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, static_cast<WUInt16>(iSubDivs), static_cast<WUInt16>(iSubDivs), true);

  WStringBuilder sName;
  sName.SetFormat("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

WMeshBufferResourceHandle WGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  WGeometry geom;
  geom.AddBox(WVec3(fWidth, fHeight, fDepth), false);

  WStringBuilder sName;
  sName.SetFormat("Box_{0}_{1}_{2}", WArgF(fWidth, 1), WArgF(fHeight, 1), WArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

WMeshBufferResourceHandle WGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  WGeometry geom;
  geom.AddLineBox(WVec3(fWidth, fHeight, fDepth));

  WStringBuilder sName;
  sName.SetFormat("LineBox_{0}_{1}_{2}", WArgF(fWidth, 1), WArgF(fHeight, 1), WArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void WGraphicsTest::RenderObject(WMeshBufferResourceHandle hObject, const WMat4& mTransform, const WColor& color, WBitflags<WShaderBindFlags> ShaderBindFlags)
{
  WRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  ObjectCB* ocb = WRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);

  WRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  WRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
}
WGALTextureHandle WGraphicsTest::GetBackbuffer() const
{
  const WGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
  return pPrimarySwapChain->GetBackBufferTexture();
}

void WGraphicsTest::TextureBarrier(const WGALTextureBarrier& barrier)
{
  m_pEncoder->TextureBarrier(WMakeArrayPtr(&barrier, 1));
}

void WGraphicsTest::BufferBarrier(const WGALBufferBarrier& barrier)
{
  m_pEncoder->BufferBarrier(WMakeArrayPtr(&barrier, 1));
}
