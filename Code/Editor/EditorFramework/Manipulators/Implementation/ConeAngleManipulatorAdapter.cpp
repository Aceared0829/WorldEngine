#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WConeAngleManipulatorAdapter::WConeAngleManipulatorAdapter() = default;

WConeAngleManipulatorAdapter::~WConeAngleManipulatorAdapter() = default;

void WConeAngleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);

  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WConeAngleManipulatorAdapter::GizmoEventHandler, this));
}

void WConeAngleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WConeAngleManipulatorAttribute* pAttr = static_cast<const WConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

  /* if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
  } */

  m_Gizmo.SetRadius(pAttr->m_fScale);

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    WAngle value = pObjectAccessor->Get<WAngle>(m_pObject, GetProperty(pAttr->GetAngleProperty()));
    m_Gizmo.SetAngle(value);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void WConeAngleManipulatorAdapter::GizmoEventHandler(const WGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
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
      const WConeAngleManipulatorAttribute* pAttr = static_cast<const WConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetAngleProperty(), m_Gizmo.GetAngle());
    }
    break;
  }
}

void WConeAngleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
