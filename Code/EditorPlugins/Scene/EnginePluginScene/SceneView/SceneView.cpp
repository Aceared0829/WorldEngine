#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Core/Utils/Blackboard.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WSceneViewContext::WSceneViewContext(WSceneContext* pSceneContext)
  : WEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_bUpdatePickingData = true;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));

  m_CullingCamera = m_Camera;
}

WSceneViewContext::~WSceneViewContext() = default;

void WSceneViewContext::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
  WEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewRedrawMsgToEngine>())
  {
    const WViewRedrawMsgToEngine* pMsg2 = static_cast<const WViewRedrawMsgToEngine*>(pMsg);

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.Active"), pMsg2->m_bUpdatePickingData);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickSelected"), pMsg2->m_bEnablePickingSelected);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickTransparent"), pMsg2->m_bEnablePickTransparent);
    }

    if (pMsg2->m_iCameraMode == WCameraMode::PerspectiveFixedFovX || pMsg2->m_iCameraMode == WCameraMode::PerspectiveFixedFovY)
    {
      if (!m_pSceneContext->IsPlayTheGameActive())
      {
        if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
        {
          pSoundInterface->SetListener(-1, pMsg2->m_vPosition, pMsg2->m_vDirForwards, pMsg2->m_vDirUp, WVec3::MakeZero());
        }
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewPickingMsgToEngine>())
  {
    const WViewPickingMsgToEngine* pMsg2 = static_cast<const WViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewMarqueePickingMsgToEngine>())
  {
    const WViewMarqueePickingMsgToEngine* pMsg2 = static_cast<const WViewMarqueePickingMsgToEngine*>(pMsg);

    MarqueePickObjects(pMsg2);
  }
}

void WSceneViewContext::SetupRenderTarget(WGALSwapChainHandle hSwapChain, const WGALRenderTargets* pRenderTargets, WUInt16 uiWidth, WUInt16 uiHeight)
{
  WEngineProcessViewContext::SetupRenderTarget(hSwapChain, pRenderTargets, uiWidth, uiHeight);
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    WTagSet& excludeTags = pView->m_ExcludeTags;
    const WArrayPtr<const WTag> addTags = m_pSceneContext->GetInvisibleLayerTags();
    for (const WTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

bool WSceneViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewRenderMode(WViewRenderMode::Default);
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorSelectionPass.Active"), false);
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorShapeIconsExtractor.Active"), false);
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorGridExtractor.Active"), false);
    pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickSelected"), true);
  }

  W_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  const WCameraComponentManager* pCamMan = m_pSceneContext->GetWorld()->GetComponentManager<WCameraComponentManager>();
  if (pCamMan)
  {
    for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
    {
      const WCameraComponent* pCamComp = it;

      if (pCamComp->GetUsageHint() == WCameraUsageHint::Thumbnail)
      {
        m_Camera.LookAt(pCamComp->GetOwner()->GetGlobalPosition(), pCamComp->GetOwner()->GetGlobalPosition() + pCamComp->GetOwner()->GetGlobalDirForwards(), pCamComp->GetOwner()->GetGlobalDirUp());

        m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, 70.0f, 0.1f, 100.0f);

        m_CullingCamera = m_Camera;
        return true;
      }
    }
  }

  bool bResult = !FocusCameraOnObject(m_Camera, bounds, 70.0f, -WVec3(5, -2, 3));
  m_CullingCamera = m_Camera;
  return bResult;
}

void WSceneViewContext::SetInvisibleLayerTags(const WArrayPtr<WTag> removeTags, const WArrayPtr<WTag> addTags)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    WTagSet& excludeTags = pView->m_ExcludeTags;
    for (const WTag& removeTag : removeTags)
    {
      excludeTags.Remove(removeTag);
    }
    for (const WTag& addTag : addTags)
    {
      excludeTags.Set(addTag);
    }
  }
}

void WSceneViewContext::Redraw(bool bRenderEditorGizmos)
{
  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    const WTag& tagNoOrtho = WTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    W_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<WGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  WEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

void WSceneViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  WEngineProcessViewContext::SetCamera(pMsg);

  WView* pView = nullptr;
  WRenderWorld::TryGetView(m_hView, pView);

  bool bDebugCulling = false;
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  bDebugCulling = WRenderPipeline::cvar_SpatialCullingVis;
#endif

  if (bDebugCulling && pView != nullptr)
  {
    if (const WCameraComponentManager* pCameraManager = pView->GetWorld()->GetComponentManager<WCameraComponentManager>())
    {
      if (const WCameraComponent* pCameraComponent = pCameraManager->GetCameraByUsageHint(WCameraUsageHint::Culling))
      {
        const WGameObject* pOwner = pCameraComponent->GetOwner();
        WVec3 vPosition = pOwner->GetGlobalPosition();
        WVec3 vForward = pOwner->GetGlobalDirForwards();
        WVec3 vUp = pOwner->GetGlobalDirUp();

        m_CullingCamera.LookAt(vPosition, vPosition + vForward, vUp);

        auto cameraMode = pCameraComponent->GetCameraMode();
        float fFovOrDim = pCameraComponent->GetFieldOfView();
        if (cameraMode == WCameraMode::OrthoFixedWidth || cameraMode == WCameraMode::OrthoFixedHeight)
        {
          fFovOrDim = pCameraComponent->GetOrthoDimension();
        }

        const float fNearPlane = pCameraComponent->GetNearPlane();
        const float fFarPlane = pCameraComponent->GetFarPlane();
        m_CullingCamera.SetCameraMode(cameraMode, fFovOrDim, fNearPlane, WMath::Max(fNearPlane + 0.00001f, fFarPlane));
      }
    }
  }
  else
  {
    m_CullingCamera = m_Camera;
  }
}

void WSceneViewContext::SetViewProperties(WView* pView)
{
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorSelectionPass.Active"), m_pSceneContext->GetRenderSelectionOverlay());
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorShapeIconsExtractor.Active"), m_pSceneContext->GetRenderShapeIcons());
}

WViewHandle WSceneViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Editor - View");

  WVariant sceneContextVariant(m_pSceneContext);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorSelectedObjectsExtractor.SceneContext"), sceneContextVariant);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorShapeIconsExtractor.SceneContext"), sceneContextVariant);
  pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorGridExtractor.SceneContext"), sceneContextVariant);

  pView->SetCullingCamera(&m_CullingCamera);

  pView->m_ExcludeTags.SetByName("EditorHidden");

  return pView->GetHandle();
}

void WSceneViewContext::PickObjectAt(WUInt16 x, WUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  WViewPickingResultMsgToEditor res;
  W_SCOPE_EXIT(SendViewMessage(&res));

  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView) == false)
    return;

  auto pBlackboard = pView->GetBlackboard();
  pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.PickingPosition"), WVec2(x, y));

  auto pPickedPositionEntry = pBlackboard->GetEntry("EditorPickingPass.PickedPosition");
  if (pPickedPositionEntry == nullptr || pPickedPositionEntry->m_Value.IsA<WVec3>() == false)
    return;

  const WUInt32 uiPickingID = pBlackboard->GetEntryValue("EditorPickingPass.PickedID").ConvertTo<WUInt32>();
  res.m_vPickedNormal = pBlackboard->GetEntryValue("EditorPickingPass.PickedNormal").ConvertTo<WVec3>();
  res.m_vPickingRayStartPosition = pBlackboard->GetEntryValue("EditorPickingPass.PickedRayStartPosition").ConvertTo<WVec3>();
  res.m_vPickedPosition = pPickedPositionEntry->m_Value.ConvertTo<WVec3>();

  W_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

  const WUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
  const WUInt32 uiPartIndex = (uiPickingID >> 24) & 0xFF;

  WArrayPtr<WWorldRttiConverterContext*> contexts = m_pSceneContext->GetAllContexts();
  for (WWorldRttiConverterContext* pContext : contexts)
  {
    res.m_ComponentGuid = pContext->m_ComponentPickingMap.GetGuid(uiComponentID);
    if (res.m_ComponentGuid.IsValid() == false)
      continue;

    WComponentHandle hComponent = pContext->m_ComponentMap.GetHandle(res.m_ComponentGuid);

    WEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

    // check whether the component is still valid
    WComponent* pComponent = nullptr;
    if (pDocumentContext->GetWorld()->TryGetComponent<WComponent>(hComponent, pComponent))
    {
      // if yes, fill out the parent game object guid
      res.m_ObjectGuid = pContext->m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
      res.m_uiPartIndex = uiPartIndex;
    }
    else
    {
      res.m_ComponentGuid = WUuid();
    }
    break;
  }

  // Always take the other picking ID from the scene itself as gizmos are handled by the window and only the scene itself has one.
  res.m_OtherGuid = m_pSceneContext->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);
}

void WSceneViewContext::MarqueePickObjects(const WViewMarqueePickingMsgToEngine* pMsg)
{
  // remote processes do not support picking, just ignore this
  if (WEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  WViewMarqueePickingResultMsgToEditor res;
  res.m_uiWhatToDo = pMsg->m_uiWhatToDo;
  res.m_uiActionIdentifier = 0;

  WView* pView = nullptr;
  if (WRenderWorld::TryGetView(m_hView, pView))
  {
    auto pBlackboard = pView->GetBlackboard();
    pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.MarqueePickPos0"), WVec2(pMsg->m_uiPickPosX0, pMsg->m_uiPickPosY0));
    pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.MarqueePickPos1"), WVec2(pMsg->m_uiPickPosX1, pMsg->m_uiPickPosY1));
    pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.MarqueeActionID"), pMsg->m_uiActionIdentifier);

    if (pMsg->m_uiWhatToDo == 0xFF)
      return;

    auto pMarqueeActionIDEntry = pBlackboard->GetEntry("EditorPickingPass.MarqueeResultActionID");
    if (pMarqueeActionIDEntry == nullptr || pMarqueeActionIDEntry->m_Value.ConvertTo<WUInt32>() != pMsg->m_uiActionIdentifier)
      return;

    res.m_uiActionIdentifier = pMsg->m_uiActionIdentifier;

    WVariant varMarquee = pBlackboard->GetEntryValue("EditorPickingPass.MarqueeResult");

    if (varMarquee.IsA<WVariantArray>())
    {
      WEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

      const WVariantArray resArray = varMarquee.Get<WVariantArray>();

      for (WUInt32 i = 0; i < resArray.GetCount(); ++i)
      {
        const WVariant& singleRes = resArray[i];

        const WUInt32 uiPickingID = singleRes.ConvertTo<WUInt32>();
        const WUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);

        const WUuid componentGuid = m_pSceneContext->GetActiveContext().m_ComponentPickingMap.GetGuid(uiComponentID);

        if (componentGuid.IsValid())
        {
          WComponentHandle hComponent = m_pSceneContext->GetActiveContext().m_ComponentMap.GetHandle(componentGuid);

          // check whether the component is still valid
          WComponent* pComponent = nullptr;
          if (pDocumentContext->GetWorld()->TryGetComponent<WComponent>(hComponent, pComponent))
          {
            // if yes, fill out the parent game object guid
            res.m_ObjectGuids.PushBack(m_pSceneContext->GetActiveContext().m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle()));
          }
        }
      }
    }

    pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.MarqueePickPos0"), WVec2(-1));
    pBlackboard->SetEntryValue(WMakeHashedString("EditorPickingPass.MarqueePickPos1"), WVec2(-1));
  }

  if (res.m_uiActionIdentifier == 0)
    return;

  SendViewMessage(&res);
}
