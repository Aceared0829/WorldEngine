#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Utilities/GraphicsUtils.h>

WSelectionContext::WSelectionContext(WQtEngineDocumentWindow* pOwnerWindow, WQtEngineViewWidget* pOwnerView, const WCamera* pCamera)
{
  m_pCamera = pCamera;

  SetOwner(pOwnerWindow, pOwnerView);

  m_hMarqueeGizmo.ConfigureHandle(nullptr, WEngineGizmoHandleType::LineBox, WColor::CadetBlue, WGizmoFlags::ShowInOrtho | WGizmoFlags::OnTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hMarqueeGizmo);
}

WSelectionContext::~WSelectionContext()
{
  // if anyone is registered for object picking, tell them that nothing was picked,
  // so that they reset their state
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride(nullptr);
    ResetPickObjectOverride();
  }
}

void WSelectionContext::SetPickObjectOverride(WDelegate<void(const WDocumentObject*)> pickOverride)
{
  m_PickObjectOverride = pickOverride;
  GetOwnerView()->setCursor(Qt::CrossCursor);
}

void WSelectionContext::ResetPickObjectOverride()
{
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride.Invalidate();
    GetOwnerView()->unsetCursor();
  }
}

WEditorInput WSelectionContext::DoMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = GetOwnerWindow()->GetDocument()->FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<WGizmoHandle>())
        {
          WGizmoHandle* pGizmoHandle = static_cast<WGizmoHandle*>(pSO);
          WGizmo* pGizmo = pGizmoHandle->GetOwnerGizmo();

          if (pGizmo)
          {
            pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_vViewport);
            return pGizmo->MousePressEvent(e);
          }
        }
      }
    }

    m_Mode = Mode::Single;

    if (m_bPressedSpace && !m_PickObjectOverride.IsValid())
    {
      m_uiMarqueeID += 23;
      m_vMarqueeStartPos.Set(e->pos().x(), e->pos().y(), 0.01f);

      // no modifier -> add, CTRL -> remove
      m_Mode = e->modifiers().testFlag(Qt::ControlModifier) ? Mode::MarqueeRemove : Mode::MarqueeAdd;
      MakeActiveInputContext();

      if (m_Mode == Mode::MarqueeAdd)
        m_hMarqueeGizmo.SetColor(WColor::LightSkyBlue);
      else
        m_hMarqueeGizmo.SetColor(WColor::PaleVioletRed);

      return WEditorInput::WasExclusivelyHandled;
    }
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WSelectionContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    if (e->modifiers() & Qt::KeyboardModifier::ControlModifier)
    {
      const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      OpenDocumentForPickedObject(res);
    }
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    if (m_Mode == Mode::Single)
    {
      const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;
      const bool bDirect = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;
      SelectPickedObject(res, bToggle, bDirect);

      DoFocusLost(false);

      // we handled the mouse click event
      // but this is it, we don't stay active
      return WEditorInput::WasExclusivelyHandled;
    }

    if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
    {
      SendMarqueeMsg(e, (m_Mode == Mode::MarqueeAdd) ? 1 : 2);

      const bool bPressedSpace = m_bPressedSpace;
      DoFocusLost(false);
      m_bPressedSpace = bPressedSpace;
      return WEditorInput::WasExclusivelyHandled;
    }
  }

  return WEditorInput::MayBeHandledByOthers;
}


void WSelectionContext::OpenDocumentForPickedObject(const WObjectPickingResult& res) const
{
  if (!res.m_PickedComponent.IsValid())
    return;

  auto* pDocument = GetOwnerWindow()->GetDocument();

  if (const WDocumentObject* pPickedComponent = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent))
  {
    for (auto pDocMan : WDocumentManager::GetAllDocumentManagers())
    {
      if (WAssetDocumentManager* pAssetMan = WDynamicCast<WAssetDocumentManager*>(pDocMan))
      {
        if (pAssetMan->OpenPickedDocument(pPickedComponent, res.m_uiPartIndex).Succeeded())
        {
          return;
        }
      }
    }

    // Fallback: iterate component properties and open the first asset-browser string property we find a value for.
    for (auto pProperty : pPickedComponent->GetTypeAccessor().GetType()->GetProperties())
    {
      const WRTTI* pType = pProperty->GetSpecificType();

      if (pProperty->GetAttributeByType<WAssetBrowserAttribute>() != nullptr && (pType == WGetStaticRTTI<const char*>() || pType == WGetStaticRTTI<WString>() || pType == WGetStaticRTTI<WStringView>()))
      {
        WStringBuilder sValue;

        if (pProperty->GetCategory() == WPropertyCategory::Member)
        {
          sValue = pPickedComponent->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<WString>();
        }
        else if (pProperty->GetCategory() == WPropertyCategory::Array)
        {
          if (pPickedComponent->GetTypeAccessor().GetCount(pProperty->GetPropertyName()) > 0)
            sValue = pPickedComponent->GetTypeAccessor().GetValue(pProperty->GetPropertyName(), 0).ConvertTo<WString>();
        }

        if (!sValue.IsEmpty() && WAssetDocumentManager::TryOpenAssetDocument(sValue).Succeeded())
          return;
      }
    }

    GetOwnerWindow()->ShowTemporaryStatusBarMsg("Could not open a document for the picked object");
  }
}

void WSelectionContext::SelectPickedObject(const WObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  if (res.m_PickedObject.IsValid())
  {
    auto* pDocument = GetOwnerWindow()->GetDocument();
    const WDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);
    if (!pObject)
      return;

    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(pObject);
    }
    else
    {
      if (bToggle)
        pDocument->GetSelectionManager()->ToggleObject(determineObjectToSelect(pObject, true, bDirect));
      else
        pDocument->GetSelectionManager()->SetSelection(determineObjectToSelect(pObject, false, bDirect));
    }
  }
}

void WSelectionContext::SendMarqueeMsg(QMouseEvent* e, WUInt8 uiWhatToDo)
{
  // Get devicePixelRatio from the owner view
  const qreal devicePixelRatio = GetOwnerView()->devicePixelRatioF();

  WVec2I32 curPos;
  curPos.Set(e->pos().x(), e->pos().y());

  WMat4 mView = m_pCamera->GetViewMatrix();
  WMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);

  WMat4 mViewProj = mProj * mView;
  WMat4 mInvViewProj = mViewProj;
  if (mInvViewProj.Invert(0.0f).Failed())
  {
    // if this fails, the marquee will not be rendered correctly
    W_ASSERT_DEBUG(false, "Failed to invert view projection matrix.");
  }

  // Multiply mouse positions by devicePixelRatio
  const WVec3 vMousePos(e->pos().x(), e->pos().y(), 0.01f);
  const WVec3 vScreenSpacePos0(vMousePos.x, vMousePos.y, vMousePos.z);
  const WVec3 vScreenSpacePos1(m_vMarqueeStartPos.x, m_vMarqueeStartPos.y, m_vMarqueeStartPos.z);

  WVec3 vPosOnNearPlane0, vRayDir0;
  WVec3 vPosOnNearPlane1, vRayDir1;
  WGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos0, vPosOnNearPlane0, &vRayDir0).IgnoreResult();
  WGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos1, vPosOnNearPlane1, &vRayDir1).IgnoreResult();

  WTransform t;
  t.SetIdentity();
  t.m_vPosition = WMath::Lerp(vPosOnNearPlane0, vPosOnNearPlane1, 0.5f);
  t.m_qRotation = WQuat::MakeFromMat3(m_pCamera->GetViewMatrix().GetRotationalPart());

  // box coordinates in screen space
  WVec3 vBoxPosSS0 = t.m_qRotation * vPosOnNearPlane0;
  WVec3 vBoxPosSS1 = t.m_qRotation * vPosOnNearPlane1;

  t.m_qRotation = t.m_qRotation.GetInverse();

  t.m_vScale.x = WMath::Abs(vBoxPosSS0.x - vBoxPosSS1.x);
  t.m_vScale.y = WMath::Abs(vBoxPosSS0.y - vBoxPosSS1.y);
  t.m_vScale.z = 0.0f;

  m_hMarqueeGizmo.SetTransformation(t);
  m_hMarqueeGizmo.SetVisible(true);

  {
    WViewMarqueePickingMsgToEngine msg;
    msg.m_uiViewID = GetOwnerView()->GetViewID();
    msg.m_uiPickPosX0 = (WUInt16)(m_vMarqueeStartPos.x * devicePixelRatio);
    msg.m_uiPickPosY0 = (WUInt16)(m_vMarqueeStartPos.y * devicePixelRatio);
    msg.m_uiPickPosX1 = (WUInt16)(e->pos().x() * devicePixelRatio);
    msg.m_uiPickPosY1 = (WUInt16)(e->pos().y() * devicePixelRatio);
    msg.m_uiWhatToDo = uiWhatToDo;
    msg.m_uiActionIdentifier = m_uiMarqueeID;

    GetOwnerView()->GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }
}

WEditorInput WSelectionContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (IsActiveInputContext() && (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove))
  {
    SendMarqueeMsg(e, 0xFF);

    return WEditorInput::WasExclusivelyHandled;
  }
  else
  {
    WViewHighlightMsgToEngine msg;

    {
      const WObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      if (res.m_PickedComponent.IsValid())
        msg.m_HighlightObject = res.m_PickedComponent;
      else if (res.m_PickedOther.IsValid())
        msg.m_HighlightObject = res.m_PickedOther;
      else
        msg.m_HighlightObject = res.m_PickedObject;
    }

    GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

    // we only updated the highlight, so others may do additional stuff, if they like
    return WEditorInput::MayBeHandledByOthers;
  }
}

WEditorInput WSelectionContext::DoKeyPressEvent(QKeyEvent* e)
{
  /// \todo Handle the current cursor (icon) across all active input contexts

  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = true;
    return WEditorInput::MayBeHandledByOthers;
  }

  if (e->key() == Qt::Key_Delete)
  {
    GetOwnerWindow()->GetDocument()->DeleteSelectedObjects();
    return WEditorInput::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_Escape)
  {
    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(nullptr);
      ResetPickObjectOverride();
    }
    else
    {
      if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
      {
        const bool bPressedSpace = m_bPressedSpace;
        FocusLost(true);
        m_bPressedSpace = bPressedSpace;
      }
      else
      {
        GetOwnerWindow()->GetDocument()->GetSelectionManager()->Clear();
      }
    }

    return WEditorInput::WasExclusivelyHandled;
  }

  return WEditorInput::MayBeHandledByOthers;
}

WEditorInput WSelectionContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = false;
  }

  return WEditorInput::MayBeHandledByOthers;
}

static const bool IsInSelection(const WDeque<const WDocumentObject*>& selection, const WDocumentObject* pObject, const WDocumentObject*& out_pParentInSelection, const WDocumentObject*& out_pParentChild, const WDocumentObject* pRootObject)
{
  if (pObject == pRootObject)
    return false;

  if (selection.IndexOf(pObject) != WInvalidIndex)
  {
    out_pParentInSelection = pObject;
    return true;
  }

  const WDocumentObject* pParent = pObject->GetParent();

  if (IsInSelection(selection, pParent, out_pParentInSelection, out_pParentChild, pRootObject))
  {
    if (out_pParentChild == nullptr)
      out_pParentChild = pObject;

    return true;
  }

  return false;
}

static const WDocumentObject* GetPrefabParentOrSelf(const WDocumentObject* pObject)
{
  const WDocumentObject* pParent = pObject;
  const WDocument* pDocument = pObject->GetDocumentObjectManager()->GetDocument();
  const auto& metaData = *pDocument->m_DocumentObjectMetaData;

  while (pParent != nullptr)
  {
    {
      const WDocumentObjectMetaData* pMeta = metaData.BeginReadMetaData(pParent->GetGuid());
      bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();
      metaData.EndReadMetaData();

      if (bIsPrefab)
        return pParent;
    }
    pParent = pParent->GetParent();
  }

  return pObject;
}

const WDocumentObject* WSelectionContext::determineObjectToSelect(const WDocumentObject* pickedObject, bool bToggle, bool bDirect) const
{
  auto* pDocument = GetOwnerWindow()->GetDocument();
  const WDeque<const WDocumentObject*> sel = pDocument->GetSelectionManager()->GetSelection();

  const WDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject();

  const WDocumentObject* pParentInSelection = nullptr;
  const WDocumentObject* pParentChild = nullptr;

  if (!IsInSelection(sel, pickedObject, pParentInSelection, pParentChild, pRootObject))
  {
    if (bDirect)
      return pickedObject;

    return GetPrefabParentOrSelf(pickedObject);
  }
  else
  {
    if (bToggle)
    {
      // always toggle the object that is already in the selection
      return pParentInSelection;
    }

    if (bDirect)
      return pickedObject;

    if (sel.GetCount() > 1)
    {
      // multi-selection, but no toggle, so we are about to set the selection
      // -> always use the top-level parent in this case
      return GetPrefabParentOrSelf(pickedObject);
    }

    if (pParentInSelection == pickedObject)
    {
      // object itself is in the selection
      return pickedObject;
    }

    if (pParentChild == nullptr)
    {
      return pParentInSelection;
    }

    return pParentChild;
  }
}

void WSelectionContext::DoFocusLost(bool bCancel)
{
  WEditorInputContext::DoFocusLost(bCancel);

  m_bPressedSpace = false;
  m_Mode = Mode::None;
  m_hMarqueeGizmo.SetVisible(false);

  if (IsActiveInputContext())
    MakeActiveInputContext(false);
}
