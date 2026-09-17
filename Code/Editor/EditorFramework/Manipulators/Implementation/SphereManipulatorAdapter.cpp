#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WSphereManipulatorAdapter::WSphereManipulatorAdapter() = default;

WSphereManipulatorAdapter::~WSphereManipulatorAdapter() = default;

void WSphereManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);

  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WSphereManipulatorAdapter::GizmoEventHandler, this));
}

void WSphereManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WSphereManipulatorAttribute* pAttr = static_cast<const WSphereManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetInnerRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetInnerRadiusProperty()));
    m_Gizmo.SetInnerSphere(true, fValue);
  }

  if (!pAttr->GetOuterRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetOuterRadiusProperty()));
    m_Gizmo.SetOuterSphere(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void WSphereManipulatorAdapter::GizmoEventHandler(const WGizmoEvent& e)
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
      const WSphereManipulatorAttribute* pAttr = static_cast<const WSphereManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetInnerRadiusProperty(), m_Gizmo.GetInnerRadius(), pAttr->GetOuterRadiusProperty(), m_Gizmo.GetOuterRadius());
    }
    break;
  }
}

void WSphereManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
