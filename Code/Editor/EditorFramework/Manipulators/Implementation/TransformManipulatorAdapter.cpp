#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WTransformManipulatorAdapter::WTransformManipulatorAdapter() = default;

WTransformManipulatorAdapter::~WTransformManipulatorAdapter() = default;

void WTransformManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);

  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_TranslateGizmo.SetTransformation(GetObjectTransform());
  m_RotateGizmo.SetTransformation(GetObjectTransform());
  m_ScaleGizmo.SetTransformation(GetObjectTransform());

  const WTransformManipulatorAttribute* pAttr = static_cast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    m_bHideTranslate = GetProperty(pAttr->GetTranslateProperty())->GetFlags().IsSet(WPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    m_bHideRotate = GetProperty(pAttr->GetRotateProperty())->GetFlags().IsSet(WPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    m_bHideScale = GetProperty(pAttr->GetScaleProperty())->GetFlags().IsSet(WPropertyFlags::ReadOnly);
  }

  m_TranslateGizmo.SetOwner(pEngineWindow, nullptr);
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTransformManipulatorAdapter::GizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTransformManipulatorAdapter::GizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WTransformManipulatorAdapter::GizmoEventHandler, this));
}

void WTransformManipulatorAdapter::Update()
{
  UpdateGizmoTransform();
}

void WTransformManipulatorAdapter::GizmoEventHandler(const WGizmoEvent& e)
{
  const WTransformManipulatorAttribute* pAttr = static_cast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr);

  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
      m_vOldScale = GetScale();
      BeginTemporaryInteraction();
      break;

    case WGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case WGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case WGizmoEvent::Type::Interaction:
    {
      if (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo == &m_RotateGizmo || e.m_pGizmo == &m_ScaleGizmo)
      {
        const WTransform tParent = GetObjectTransform();
        const WTransform tGlobal = static_cast<const WGizmo*>(e.m_pGizmo)->GetTransformation();
        WTransform tLocal;
        tLocal = WTransform::MakeLocalTransform(tParent, tGlobal);
        if (e.m_pGizmo == &m_TranslateGizmo)
        {
          ChangeProperties(pAttr->GetTranslateProperty(), tLocal.m_vPosition);
        }
        else if (e.m_pGizmo == &m_RotateGizmo)
        {
          ChangeProperties(pAttr->GetRotateProperty(), tLocal.m_qRotation);
        }
        else if (e.m_pGizmo == &m_ScaleGizmo)
        {
          WVec3 vNewScale = m_vOldScale.CompMul(m_ScaleGizmo.GetScalingResult());
          ChangeProperties(pAttr->GetScaleProperty(), vNewScale);
        }
      }
    }
    break;
  }
}


void WTransformManipulatorAdapter::UpdateGizmoTransform()
{
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  const WVec3 vPos = GetTranslation();
  const WQuat vRot = GetRotation();
  const WVec3 vScale = GetScale();

  const WTransform tParent = GetObjectTransform();
  WTransform tLocal;
  tLocal.m_vPosition = vPos;
  tLocal.m_qRotation = vRot;
  tLocal.m_vScale = vScale;
  WTransform tGlobal;
  tGlobal = WTransform::MakeGlobalTransform(tParent, tLocal);
  // Let's not apply scaling to the gizmos.
  tGlobal.m_vScale = WVec3(1, 1, 1);

  m_TranslateGizmo.SetTransformation(tGlobal);
  m_RotateGizmo.SetTransformation(tGlobal);
  m_ScaleGizmo.SetTransformation(tGlobal);
}

WVec3 WTransformManipulatorAdapter::GetTranslation()
{
  const WTransformManipulatorAttribute* pAttr = static_cast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetTranslateProperty()));
  }

  return WVec3(0);
}

WQuat WTransformManipulatorAdapter::GetRotation()
{
  const WTransformManipulatorAttribute* pAttr = static_cast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<WQuat>(m_pObject, GetProperty(pAttr->GetRotateProperty()));
  }

  return WQuat::MakeIdentity();
}

WVec3 WTransformManipulatorAdapter::GetScale()
{
  const WTransformManipulatorAttribute* pAttr = static_cast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetScaleProperty()));
  }

  return WVec3(1);
}

WTransform WTransformManipulatorAdapter::GetOffsetTransform() const
{
  WTransform offset;
  offset.SetIdentity();

  if (const WTransformManipulatorAttribute* pAttr = WDynamicCast<const WTransformManipulatorAttribute*>(m_pManipulatorAttr))
  {
    if (!pAttr->GetGetOffsetTranslationProperty().IsEmpty())
    {
      WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_vPosition = pObjectAccessor->Get<WVec3>(m_pObject, GetProperty(pAttr->GetGetOffsetTranslationProperty()));
    }

    if (!pAttr->GetGetOffsetRotationProperty().IsEmpty())
    {
      WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_qRotation = pObjectAccessor->Get<WQuat>(m_pObject, GetProperty(pAttr->GetGetOffsetRotationProperty()));
    }
  }

  return offset;
}
