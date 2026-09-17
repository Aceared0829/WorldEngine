#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectGizmoEditTool, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WGameObjectGizmoEditTool::WGameObjectGizmoEditTool()
{
  WQtDocumentWindow::s_Events.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

WGameObjectGizmoEditTool::~WGameObjectGizmoEditTool()
{
  WQtDocumentWindow::s_Events.RemoveEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

void WGameObjectGizmoEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::GameObjectEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
  WManipulatorManager::GetSingleton()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
  GetWindow()->m_EngineWindowEvent.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::EngineWindowEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::ObjectStructureEventHandler, this));

  // subscribe to all views that already exist
  for (WQtEngineViewWidget* pView : GetWindow()->GetViewWidgets())
  {
    if (WQtGameObjectViewWidget* pViewWidget = qobject_cast<WQtGameObjectViewWidget*>(pView))
    {
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(
        WMakeDelegate(&WGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
    }
  }
}

void WGameObjectGizmoEditTool::UpdateGizmoSelectionList()
{
  GetDocument()->ComputeTopLevelSelectedGameObjects(m_GizmoSelection);
}

void WGameObjectGizmoEditTool::UpdateGizmoVisibleState()
{
  bool isVisible = false;

  if (IsActive())
  {
    WGameObjectDocument* pDocument = GetDocument();

    const auto& selection = pDocument->GetSelectionManager()->GetSelection();

    if (selection.IsEmpty() || !selection.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
      goto done;

    isVisible = true;
    UpdateGizmoTransformation();
  }

done:
  ApplyGizmoVisibleState(isVisible);
}

void WGameObjectGizmoEditTool::UpdateGizmoTransformation()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == WGetStaticRTTI<WGameObject>())
  {
    const WTransform tGlobal = GetDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const WVec3 vPivotPoint =
      tGlobal.m_qRotation * WVec3::MakeZero(); // LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<WVec3>();

    WTransform mt;
    mt.SetIdentity();

    if (GetDocument()->GetGizmoWorldSpace() && GetSupportedSpaces() != WEditToolSupportedSpaces::LocalSpaceOnly)
    {
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }
    else
    {
      mt.m_qRotation = tGlobal.m_qRotation;
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }

    ApplyGizmoTransformation(mt);
  }
}

void WGameObjectGizmoEditTool::DocumentWindowEventHandler(const WQtDocumentWindowEvent& e)
{
  if (e.m_Type == WQtDocumentWindowEvent::WindowClosing && e.m_pWindow == GetWindow())
  {
    GetDocument()->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::GameObjectEventHandler, this));
    GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
    WManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(
      WMakeDelegate(&WGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
    GetWindow()->m_EngineWindowEvent.RemoveEventHandler(WMakeDelegate(&WGameObjectGizmoEditTool::EngineWindowEventHandler, this));
    GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
      WMakeDelegate(&WGameObjectGizmoEditTool::ObjectStructureEventHandler, this));
  }
}

void WGameObjectGizmoEditTool::UpdateManipulatorVisibility()
{
  WManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);
}

void WGameObjectGizmoEditTool::GameObjectEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::ActiveEditToolChanged:
    case WGameObjectEvent::Type::GizmoTransformMayBeInvalid:
      UpdateGizmoVisibleState();
      UpdateManipulatorVisibility();
      break;

    default:
      break;
  }
}

void WGameObjectGizmoEditTool::CommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case WCommandHistoryEvent::Type::UndoEnded:
    case WCommandHistoryEvent::Type::RedoEnded:
    case WCommandHistoryEvent::Type::TransactionEnded:
    case WCommandHistoryEvent::Type::TransactionCanceled:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void WGameObjectGizmoEditTool::SelectionManagerEventHandler(const WSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case WSelectionManagerEvent::Type::SelectionCleared:
      m_GizmoSelection.Clear();
      UpdateGizmoVisibleState();
      break;

    case WSelectionManagerEvent::Type::SelectionSet:
    case WSelectionManagerEvent::Type::ObjectAdded:
      W_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");
      UpdateGizmoVisibleState();
      break;

    case WSelectionManagerEvent::Type::ObjectRemoved:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void WGameObjectGizmoEditTool::ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() &&
      !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void WGameObjectGizmoEditTool::EngineWindowEventHandler(const WEngineWindowEvent& e)
{
  if (WQtGameObjectViewWidget* pViewWidget = qobject_cast<WQtGameObjectViewWidget*>(e.m_pView))
  {
    switch (e.m_Type)
    {
      case WEngineWindowEvent::Type::ViewCreated:
        pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(
          WMakeDelegate(&WGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
        break;

      default:
        break;
    }
  }
}

void WGameObjectGizmoEditTool::ObjectStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (!IsActive() || m_bInGizmoInteraction)
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void WGameObjectGizmoEditTool::TransformationGizmoEventHandler(const WGizmoEvent& e)
{
  if (!IsActive())
    return;

  WObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();

  switch (e.m_Type)
  {
    case WGizmoEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;

      TransformationGizmoEventHandlerImpl(e);

      UpdateGizmoSelectionList();

      pAccessor->BeginTemporaryCommands("Transform Object");
    }
    break;

    case WGizmoEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      pAccessor->StartTransaction("Transform Object");

      TransformationGizmoEventHandlerImpl(e);

      m_bInGizmoInteraction = false;
    }
    break;

    case WGizmoEvent::Type::EndInteractions:
    {
      pAccessor->FinishTemporaryCommands();
      m_GizmoSelection.Clear();

      if (m_bMergeTransactions)
        GetDocument()->GetCommandHistory()->MergeLastTwoTransactions(); // #TODO: this should be interleaved transactions
    }
    break;

    case WGizmoEvent::Type::CancelInteractions:
    {
      pAccessor->CancelTemporaryCommands();
      m_GizmoSelection.Clear();
    }
    break;

    default:
      break;
  }
}
