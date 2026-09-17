#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>

WEngineProcessViewContext::WEngineProcessViewContext(WEngineProcessDocumentContext* pContext)
  : m_pDocumentContext(pContext)
{
  m_uiViewID = 0xFFFFFFFF;
}

WEngineProcessViewContext::~WEngineProcessViewContext()
{
  WRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();

  WWindowManager::GetSingleton()->CloseAll(this);
}

void WEngineProcessViewContext::SetViewID(WUInt32 uiId)
{
  W_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once");
  m_uiViewID = uiId;
}

void WEngineProcessViewContext::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP) || W_ENABLED(W_PLATFORM_LINUX)
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewRedrawMsgToEngine>())
  {
    const WViewRedrawMsgToEngine* pMsg2 = static_cast<const WViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
#  if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
      HandleWindowUpdate(reinterpret_cast<WWindowHandle>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  else
      WWindowHandle windowHandle;
      windowHandle.type = WWindowHandle::Type::XCB;
      windowHandle.xcbWindow.m_Window = static_cast<WUInt32>(pMsg2->m_uiHWND);
      windowHandle.xcbWindow.m_pConnection = nullptr;
      HandleWindowUpdate(windowHandle, pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  endif
      Redraw(true);
    }
  }
  else if (const WViewScreenshotMsgToEngine* msg = WDynamicCast<const WViewScreenshotMsgToEngine*>(pMsg))
  {
    auto* pOutputTarget = WWindowManager::GetSingleton()->GetOutputTarget(m_hEditorWindow);
    if (pOutputTarget->StartCaptureImage().Succeeded())
    {
      m_sPendingScreenshotPath = msg->m_sOutputFile;
    }
  }
#else
#  error "Unsupported platform."
#endif
}

void WEngineProcessViewContext::SendViewMessage(WEditorEngineViewMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();
  pViewMsg->m_uiViewID = m_uiViewID;

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void WEngineProcessViewContext::HandleWindowUpdate(WWindowHandle hWnd, WUInt16 uiWidth, WUInt16 uiHeight)
{
  W_LOG_BLOCK("WEngineProcessViewContext::HandleWindowUpdate");

  auto pWinMan = WWindowManager::GetSingleton();

  if (!m_hEditorWindow.IsInvalidated())
  {
    // Update window size
    auto* pWindow = static_cast<WEditorProcessViewWindow*>(pWinMan->GetWindow(m_hEditorWindow));
    const WSizeU32 wndSize = pWindow->GetClientAreaSize();

    W_ASSERT_DEV(pWindow->GetNativeWindowHandle() == hWnd, "Editor view handle must never change. View needs to be destroyed and recreated.");

    if (wndSize.width == uiWidth && wndSize.height == uiHeight)
      return;

    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      WLog::Error("Failed to update Editor Process View Window");
    }
    return;
  }

  // create window
  {
    WUniquePtr<WEditorProcessViewWindow> pWindow = W_DEFAULT_NEW(WEditorProcessViewWindow);
    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      WLog::Error("Failed to create Editor Process View Window");
      return;
    }

    // create output target
    WUniquePtr<WWindowOutputTargetGAL> pOutput = W_DEFAULT_NEW(WWindowOutputTargetGAL, [this](WGALSwapChainHandle hSwapChain, WSizeU32 size)
      { OnSwapChainChanged(hSwapChain, size); });

    WGALWindowSwapChainCreationDescription desc;
    desc.m_pWindow = pWindow.Borrow();
    desc.m_BackBufferFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;

    pOutput->CreateSwapchain(desc);
    if (pOutput->m_hSwapChain.IsInvalidated())
    {
      WLog::Error("Failed to create swapchain for Editor Process View Window");
      return;
    }

    // setup render target
    {
      const WSizeU32 wndSize = pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_hSwapChain, nullptr, static_cast<WUInt16>(wndSize.width), static_cast<WUInt16>(wndSize.height));
    }

    // The document guid goes into the name because this process registers one window per open document
    // window, and they are otherwise indistinguishable: anything that picks a window by index or by
    // name (app_info and app_screenshot over MCP) could only ever guess which document it got.
    WStringBuilder sWindowName("EditorView");

    if (m_pDocumentContext != nullptr)
    {
      WStringBuilder sGuid;
      WConversionUtils::ToString(m_pDocumentContext->GetDocumentGuid(), sGuid);
      sWindowName.AppendFormat(" {}", sGuid);
    }

    m_hEditorWindow = pWinMan->Register(sWindowName, this, std::move(pWindow));
    pWinMan->SetOutputTarget(m_hEditorWindow, std::move(pOutput));
  }
}

void WEngineProcessViewContext::OnSwapChainChanged(WGALSwapChainHandle hSwapChain, WSizeU32 size)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewport(WRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
    pView->ForceUpdate();
  }
}

void WEngineProcessViewContext::SetupRenderTarget(WGALSwapChainHandle hSwapChain, const WGALRenderTargets* pRenderTargets, WUInt16 uiWidth, WUInt16 uiHeight)
{
  W_LOG_BLOCK("WEngineProcessViewContext::SetupRenderTarget");
  W_ASSERT_DEV((!hSwapChain.IsInvalidated() && pRenderTargets == nullptr) || (hSwapChain.IsInvalidated() && pRenderTargets != nullptr), "hSwapChain and pRenderTargets are mutually exclusive.");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      if (!hSwapChain.IsInvalidated())
        pView->SetSwapChain(hSwapChain);
      else
        pView->SetRenderTargets(*pRenderTargets);

      pView->SetViewport(WRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void WEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    if (!bRenderEditorGizmos)
    {
      // exclude all editor objects from rendering in proper game views
      pView->m_ExcludeTags.Set(tagEditor);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagEditor);
    }

    WRenderWorld::AddMainView(m_hView);
  }
}

bool WEngineProcessViewContext::PendingOperationInProgress() const
{
  if (m_sPendingScreenshotPath.IsEmpty())
    return false;

  auto* pOutputTarget = WWindowManager::GetSingleton()->GetOutputTarget(m_hEditorWindow);
  if (pOutputTarget == nullptr)
  {
    const_cast<WEngineProcessViewContext*>(this)->m_sPendingScreenshotPath.Clear();
    return false;
  }

  WImage img;
  WEnum<WCaptureImageResult> res = pOutputTarget->WaitCaptureImage(img);

  if (res == WCaptureImageResult::Pending)
    return true;

  if (res == WCaptureImageResult::Ready)
  {
    img.SaveTo(m_sPendingScreenshotPath).IgnoreResult();
  }

  const_cast<WEngineProcessViewContext*>(this)->m_sPendingScreenshotPath.Clear();
  return false;
}

bool WEngineProcessViewContext::FocusCameraOnObject(WCamera& inout_camera, const WBoundingBoxSphere& objectBounds, float fFov, const WVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  WVec3 vDir = vViewDir;
  bool bChanged = false;
  WVec3 vCameraPos = inout_camera.GetCenterPosition();
  WVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = WMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / WMath::Sin(WAngle::MakeFromDegree(fFov / 2));
  vDir.Normalize();
  WVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    inout_camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    inout_camera.LookAt(vNewCameraPos, vCenterPos, WVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void WEngineProcessViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  WViewRenderMode::Enum renderMode = (WViewRenderMode::Enum)pMsg->m_uiRenderMode;

  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView) && pView->GetWorld() != nullptr)
  {
    if (renderMode == WViewRenderMode::None)
    {
      pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
    }
    else
    {
      pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
    }
  }

  if (m_Camera.GetCameraMode() != WCameraMode::Stereo)
  {
    bool bCameraIsActive = false;
    if (pView && pView->GetWorld())
    {
      WEnum<WCameraUsageHint> usageHint = pView->GetCameraUsageHint();
      WCameraComponent* pComp = pView->GetWorld()->GetOrCreateComponentManager<WCameraComponentManager>()->GetCameraByUsageHint(usageHint);
      bCameraIsActive = pComp != nullptr && pComp->IsActive();
    }

    // Camera mode should be controlled by a matching camera component if one exists.
    if (!bCameraIsActive)
    {
      WCameraMode::Enum cameraMode = (WCameraMode::Enum)pMsg->m_iCameraMode;
      m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
    }

    // prevent too large values
    // sometimes this can happen when imported data is badly scaled and thus way too large
    // then adding dirForwards result in no change and we run into other asserts later
    WVec3 pos = pMsg->m_vPosition;
    pos.x = WMath::Clamp(pos.x, -1000000.0f, +1000000.0f);
    pos.y = WMath::Clamp(pos.y, -1000000.0f, +1000000.0f);
    pos.z = WMath::Clamp(pos.z, -1000000.0f, +1000000.0f);

    m_Camera.LookAt(pos, pos + pMsg->m_vDirForwards, pMsg->m_vDirUp);
  }

  if (pView)
  {
    pView->SetViewRenderMode(renderMode);

    bool bUseDepthPrePass = renderMode != WViewRenderMode::WireframeColor && renderMode != WViewRenderMode::WireframeMonochrome;
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("DepthPrePass.Active"), bUseDepthPrePass);
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("AOPass.Active"), bUseDepthPrePass); // Also disable SSAO to save some performance

    SetViewProperties(pView);
  }
}


void WEngineProcessViewContext::SetViewProperties(WView* pView)
{
  // by default this stuff is disabled, derived classes can enable it
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorSelectionPass.Active"), false);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorShapeIconsExtractor.Active"), false);
}

WRenderPipelineResourceHandle WEngineProcessViewContext::CreateDefaultRenderPipeline()
{
  return WEditorEngineProcessApp::GetSingleton()->CreateDefaultMainRenderPipeline();
}

WRenderPipelineResourceHandle WEngineProcessViewContext::CreateDebugRenderPipeline()
{
  return WEditorEngineProcessApp::GetSingleton()->CreateDefaultDebugRenderPipeline();
}

WView* WEngineProcessViewContext::CreateDefaultView(WStringView sName)
{
  WView* pView = nullptr;
  WRenderWorld::CreateView(sName, pView);
  pView->SetCameraUsageHint(WCameraUsageHint::EditorView);

  pView->SetBlackboard(WBlackboard::Create(sName));

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  WEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);

  return pView;
}

void WEngineProcessViewContext::DrawSimpleGrid() const
{
  WDynamicArray<WDebugRendererLine> lines;
  lines.Reserve(2 * (10 + 1 + 10) + 4);

  const WColor xAxisColor = WColorScheme::LightUI(WColorScheme::Red) * 0.7f;
  const WColor yAxisColor = WColorScheme::LightUI(WColorScheme::Green) * 0.7f;
  const WColor gridColor = WColorScheme::LightUI(WColorScheme::Gray) * 0.5f;

  // arrows

  const float f = 1.0f;

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, 0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, -0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(-0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    const float x = 10.0f;

    for (WInt32 y = -10; y <= +10; ++y)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)-x, (float)y, 0.0f);
      line.m_end.Set((float)+x, (float)y, 0.0f);

      if (y == 0)
      {
        line.m_startColor = xAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  {
    const float y = 10.0f;

    for (WInt32 x = -10; x <= +10; ++x)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)x, (float)-y, 0.0f);
      line.m_end.Set((float)x, (float)+y, 0.0f);

      if (x == 0)
      {
        line.m_startColor = yAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  WDebugRenderer::DrawLines(m_hView, lines, WColor::White);
}

#if W_ENABLED(W_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Win/EngineProcessViewContext_win.h>
#elif W_ENABLED(W_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
