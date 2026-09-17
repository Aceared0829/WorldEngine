#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

WCapsuleManipulatorAdapter::WCapsuleManipulatorAdapter() = default;

WCapsuleManipulatorAdapter::~WCapsuleManipulatorAdapter() = default;

void WCapsuleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = WQtDocumentWindow::FindWindowByDocument(pDoc);

  WQtEngineDocumentWindow* pEngineWindow = qobject_cast<WQtEngineDocumentWindow*>(pWindow);
  W_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(WMakeDelegate(&WCapsuleManipulatorAdapter::GizmoEventHandler, this));
}

void WCapsuleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  WObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const WCapsuleManipulatorAttribute* pAttr = static_cast<const WCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetLengthProperty()));
    m_Gizmo.SetLength(fValue);
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void WCapsuleManipulatorAdapter::GizmoEventHandler(const WGizmoEvent& e)
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
      const WCapsuleManipulatorAttribute* pAttr = static_cast<const WCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetLengthProperty(), m_Gizmo.GetLength(), pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void WCapsuleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
