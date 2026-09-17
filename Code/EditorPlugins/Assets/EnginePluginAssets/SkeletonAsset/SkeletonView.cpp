#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WSkeletonViewContext::WSkeletonViewContext(WSkeletonContext* pContext)
  : WEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(WCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(WVec3(1, 1, 1), WVec3::MakeZero(), WVec3(0.0f, 0.0f, 1.0f));
}

WSkeletonViewContext::~WSkeletonViewContext() = default;

bool WSkeletonViewContext::UpdateThumbnailCamera(const WBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -WVec3(5, -2, 3));
}


void WSkeletonViewContext::Redraw(bool bRenderEditorGizmos)
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

WViewHandle WSkeletonViewContext::CreateView()
{
  WView* pView = CreateDefaultView("Skeleton Editor - View");
  return pView->GetHandle();
}

void WSkeletonViewContext::SetCamera(const WViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    WEngineProcessViewContext::DrawSimpleGrid();
  }

  WEngineProcessViewContext::SetCamera(pMsg);

  const WUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    WResourceLock<WSkeletonResource> pSkeleton(hSkeleton, WResourceAcquireMode::AllowLoadingFallback);

    WUInt32 uiNumJoints = pSkeleton->GetDescriptor().m_Skeleton.GetJointCount();

    WStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    WDebugRenderer::Draw2DText(m_hView, sText, WVec2I32(10, viewHeight - 10), WColor::White, 16, WDebugTextHAlign::Left,
      WDebugTextVAlign::Bottom);
  }
}

void WSkeletonViewContext::HandleViewMessage(const WEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WViewPickingMsgToEngine>())
  {
    const WViewPickingMsgToEngine* pMsg2 = static_cast<const WViewPickingMsgToEngine*>(pMsg);

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hView, pView))
    {
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.Active"), true);
      pView->GetBlackboard()->SetEntryValue(WMakeHashedString("EditorPickingPass.PickSelected"), true);
    }

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else
  {
    WEngineProcessViewContext::HandleViewMessage(pMsg);
  }
}

void WSkeletonViewContext::PickObjectAt(WUInt16 x, WUInt16 y)
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

  res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
  res.m_OtherGuid = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

  if (res.m_ComponentGuid.IsValid())
  {
    WComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

    WEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

    // check whether the component is still valid
    WComponent* pComponent = nullptr;
    if (pDocumentContext->GetWorld()->TryGetComponent<WComponent>(hComponent, pComponent))
    {
      // if yes, fill out the parent game object guid
      res.m_ObjectGuid = GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
      res.m_uiPartIndex = uiPartIndex;
    }
    else
    {
      res.m_ComponentGuid = WUuid();
    }
  }
}
