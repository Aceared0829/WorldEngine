#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>
#include <QClipboard>
#include <QMenu>
#include <RendererCore/Components/CameraComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneDocument, 8, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

void WSceneDocument_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e)
{
  static const WRTTI* pRtti = WRTTI::FindTypeByName("WGameObject");

  if (e.m_pObject->GetDocumentObjectManager()->GetDocument()->GetDocumentTypeName() != "Prefab")
    return;

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto pParent = e.m_pObject->GetParent();
  if (pParent != nullptr)
  {
    if (pParent->GetTypeAccessor().GetType() == pRtti)
      return;
  }

  const WString name = e.m_pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>();
  if (name != "<Prefab-Root>")
    return;

  auto& props = *e.m_pPropertyStates;
  props["Name"].m_sNewLabelText = "Prefab.NameLabel";
  props["Active"].m_Visibility = WPropertyUiState::Invisible;
  props["LocalPosition"].m_Visibility = WPropertyUiState::Invisible;
  props["LocalRotation"].m_Visibility = WPropertyUiState::Invisible;
  props["LocalScaling"].m_Visibility = WPropertyUiState::Invisible;
  props["LocalUniformScaling"].m_Visibility = WPropertyUiState::Invisible;
  props["GlobalKey"].m_Visibility = WPropertyUiState::Invisible;
  // props["Tags"].m_Visibility = WPropertyUiState::Invisible;
}

WSceneDocument::WSceneDocument(WStringView sDocumentPath, DocumentType documentType)
  : WGameObjectDocument(sDocumentPath, W_DEFAULT_NEW(WSceneObjectManager))
{
  m_DocumentType = documentType;
  m_GameMode = GameMode::Off;
  SetAddAmbientLight(IsPrefab());

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers = false;

  GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WSceneDocument::SelectionManagerEventHandler, this), m_SelectionHandlerUnsubscriber);
}

void WSceneDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // (Local mirror only mirrors settings)
  m_ObjectMirror.SetFilterFunction([pManager = GetObjectManager()](const WDocumentObject* pObject, WStringView sProperty) -> bool
    { return pManager->IsUnderRootProperty("Settings", pObject, sProperty); });
  // (Remote IPC mirror only sends scene)
  m_pMirror->SetFilterFunction([pManager = GetObjectManager()](const WDocumentObject* pObject, WStringView sProperty) -> bool
    { return pManager->IsUnderRootProperty("Children", pObject, sProperty); });

  EnsureSettingsObjectExist();

  m_DocumentObjectMetaData->m_DataModifiedEvent.AddEventHandler(WMakeDelegate(&WSceneDocument::DocumentObjectMetaDataEventHandler, this));
  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WSceneDocument::ToolsProjectEventHandler, this));
  WEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(WMakeDelegate(&WSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();

  GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WSceneDocument::ChildOrderStructureEventHandler, this), m_ChildOrderStructureEventUnsubscriber);

  // Layer sub-documents have their storage swapped into the main document's command history when active,
  // so subscribe to the main document's command history to catch those transaction events.
  WCommandHistory* pHistory = IsMainDocument() ? GetCommandHistory() : static_cast<WSceneDocument*>(GetMainDocument())->GetCommandHistory();
  pHistory->m_Events.AddEventHandler(WMakeDelegate(&WSceneDocument::ChildOrderCommandHistoryEventHandler, this), m_ChildOrderCommandHistoryUnsubscriber);
}

WSceneDocument::~WSceneDocument()
{
  m_SelectionHandlerUnsubscriber.Unsubscribe();

  m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(WMakeDelegate(&WSceneDocument::DocumentObjectMetaDataEventHandler, this));

  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WSceneDocument::ToolsProjectEventHandler, this));

  WEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(WMakeDelegate(&WSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}

void WSceneDocument::GroupSelection()
{
  const auto& sel = GetSelectionManager()->GetSelection();
  const WUInt32 numSel = sel.GetCount();
  if (numSel <= 1)
    return;

  const WDocumentObject* pCommonParent = sel[0]->GetParent();

  // this happens for top-level objects, their parent object is an WDocumentRootObject
  if (pCommonParent->GetType() != WGetStaticRTTI<WGameObject>())
  {
    pCommonParent = nullptr;
  }

  const WTransform tGroup = GetGlobalTransform(GetSelectionManager()->GetCurrentObject());

  for (const auto& item : sel)
  {
    if (pCommonParent != item->GetParent())
    {
      pCommonParent = nullptr;
    }
  }

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  WUuid groupObj = WUuid::MakeUuid();

  WAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid = groupObj;
  cmdAdd.m_pType = WGetStaticRTTI<WGameObject>();
  cmdAdd.m_Index = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd).AssertSuccess();

  // put the new group object under the shared parent
  if (pCommonParent != nullptr)
  {
    WMoveObjectCommand cmdMove;
    cmdMove.m_NewParent = pCommonParent->GetGuid();
    cmdMove.m_Index = -1;
    cmdMove.m_sParentProperty = "Children";

    cmdMove.m_Object = cmdAdd.m_NewObjectGuid;
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, tGroup, TransformationChanges::All);

  WMoveObjectCommand cmdMove;
  cmdMove.m_NewParent = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove).AssertSuccess();
  }

  pHistory->FinishTransaction();

  const WDocumentObject* pGroupObj = GetObjectManager()->GetObject(groupObj);

  GetSelectionManager()->SetSelection(pGroupObj);

  ShowDocumentStatus(WFmt("Grouped {} objects", numSel));
}

void WSceneDocument::SelectParentObject()
{
  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty())
    return;

  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  const WDocumentObject* pObject = GetObjectManager()->GetObject(Sel[0]->GetGuid());

  if (pObject->GetParent() && pObject->GetParent() != GetObjectManager()->GetRootObject())
  {
    GetSelectionManager()->SetSelection(pObject->GetParent());
  }
  else
  {
    ShowDocumentStatus("Object has no parent.");
  }
}

void WSceneDocument::SetSelectedAsActiveParent()
{
  const auto& sel = GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;

  SetActiveParent(sel.PeekBack()->GetGuid());
}

void WSceneDocument::ClearActiveParent()
{
  SetActiveParent(WUuid::MakeInvalid());
}

void WSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  WQtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  WMap<WUuid, WUuid> parents;

  WAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  WStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", WConversionUtils::ToString(it.Key(), tmp1), WConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  WAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  WDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;
  cmd.m_uiNumberOfCopies = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation = dlg.s_vRotationStep;
  cmd.m_vRandomRotation = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle = WAngle::MakeFromDegree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep = WAngle::MakeFromDegree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void WSceneDocument::DeltaTransform()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  WQtDeltaTransformDlg dlg(nullptr, this);
  dlg.exec();
}

void WSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  WMat3 mRot;

  WTransform transform;
  transform.m_vScale.Set(1.0f);
  transform.m_vPosition = camera.GetCenterPosition();
  mRot.SetColumn(0, camera.GetCenterDirForwards());
  mRot.SetColumn(1, camera.GetCenterDirRight());
  mRot.SetColumn(2, camera.GetCenterDirUp());
  transform.m_qRotation = WQuat::MakeFromMat3(mRot);

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Snap Object to Camera");
  {
    for (const WDocumentObject* pObject : selection)
    {
      SetGlobalTransform(pObject, transform, TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }
  pHistory->FinishTransaction();
}


void WSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr || !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  if (GetObjectManager()->GetObject(ctxt.m_pLastPickingResult->m_PickedObject) == nullptr)
  {
    WQtUiServices::GetSingleton()->MessageBoxStatus(WStatus(W_FAILURE), "Target object belongs to a different document.");
    return;
  }

  WMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent = ctxt.m_pLastPickingResult->m_PickedObject;
  cmd.m_Index = -1;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const WDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Attach to object failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}

void WSceneDocument::DetachFromParent()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  WMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Detach from Parent");
  {
    for (const WDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxStatus(res, "Detach from parent failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();

  ShowDocumentStatus(WFmt("Detached {} objects", selection.GetCount()));

  // reapply the selection to fix tree views etc. after the re-parenting
  WDeque<const WDocumentObject*> prevSelection = selection;
  GetSelectionManager()->Clear();
  GetSelectionManager()->SetSelection(prevSelection);
}

void WSceneDocument::SyncChildOrderForObject(const WDocumentObject* pObj)
{
  for (const WDocumentObject* pComp : pObj->GetChildren())
  {
    if (pComp->GetParentProperty() != "Components")
      continue;

    if (pComp->GetType()->GetAttributeByType<WSyncChildOrderAttribute>() == nullptr)
      continue;

    WSyncChildOrderMsgToEngine msg;
    // Sub-documents (layers) must use the main document's GUID and connection.
    WSceneDocument* pMainDoc = IsMainDocument() ? this : static_cast<WSceneDocument*>(GetMainDocument());
    msg.m_DocumentGuid = pMainDoc->GetGuid();
    msg.m_LayerGuid = IsMainDocument() ? WUuid() : GetGuid();
    msg.m_ComponentGuid = pComp->GetGuid();

    const WIReflectedTypeAccessor& accessor = pObj->GetTypeAccessor();
    const WInt32 iCount = accessor.GetCount("Children");
    for (WInt32 i = 0; i < iCount; ++i)
    {
      WVariant val = accessor.GetValue("Children", i);
      if (val.IsA<WUuid>())
        msg.m_ChildOrder.PushBack(val.Get<WUuid>());
    }

    if (!msg.m_ChildOrder.IsEmpty())
    {
      pMainDoc->GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;
  }
}

void WSceneDocument::SyncChildOrderForSelection()
{
  for (const WDocumentObject* pObj : GetSelectionManager()->GetSelection())
    SyncChildOrderForObject(pObj);
}

void WSceneDocument::SyncAllChildOrders()
{
  ApplyRecursive(GetObjectManager()->GetRootObject(), [this](const WDocumentObject* pObj)
    { SyncChildOrderForObject(pObj); });
}

void WSceneDocument::SendPendingChildOrderSyncs()
{
  if (m_PendingChildOrderSync.IsEmpty())
    return;

  for (const WUuid& guid : m_PendingChildOrderSync)
  {
    const WDocumentObject* pObj = GetObjectManager()->GetObject(guid);
    if (pObj != nullptr)
      SyncChildOrderForObject(pObj);
  }

  m_PendingChildOrderSync.Clear();
}

void WSceneDocument::ChildOrderStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    {
      if (e.m_sParentProperty != "Children")
        break;

      const WDocumentObject* pParent =
        (e.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectRemoved) ? e.m_pPreviousParent : e.m_pNewParent;

      if (pParent == nullptr)
        break;

      for (const WDocumentObject* pComp : pParent->GetChildren())
      {
        if (pComp->GetParentProperty() != "Components")
          continue;
        if (pComp->GetType()->GetAttributeByType<WSyncChildOrderAttribute>() != nullptr)
        {
          m_PendingChildOrderSync.Insert(pParent->GetGuid());
          break;
        }
      }
    }
    break;
    default:
      break;
  }
}

void WSceneDocument::ChildOrderCommandHistoryEventHandler(const WCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case WCommandHistoryEvent::Type::TransactionEnded:
    case WCommandHistoryEvent::Type::UndoEnded:
    case WCommandHistoryEvent::Type::RedoEnded:
      SendPendingChildOrderSyncs();
      break;
    case WCommandHistoryEvent::Type::TransactionCanceled:
      m_PendingChildOrderSync.Clear();
      break;
    default:
      break;
  }
}

void WSceneDocument::CopyReference()
{
  if (GetSelectionManager()->GetSelection().GetCount() != 1)
    return;

  const WUuid guid = GetSelectionManager()->GetSelection()[0]->GetGuid();

  WStringBuilder sGuid;
  WConversionUtils::ToString(guid, sGuid);

  QApplication::clipboard()->setText(sGuid.GetData());

  WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Copied Object Reference: {}", sGuid), WTime::MakeFromSeconds(5));
}

WStatus WSceneDocument::CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition, bool bComponentSelectionMenu)
{
  const WRTTI* pComponentType = WRTTI::FindTypeByName("WShapeIconComponent");

  if (bComponentSelectionMenu)
  {
    // show the context menu to select a component type

    QMenu m;
    WQtTypeMenu tm;
    tm.FillMenu(&m, WGetStaticRTTI<WComponent>(), true, false);

    m.exec(QCursor::pos());

    if (tm.m_pLastSelectedType)
    {
      pComponentType = tm.m_pLastSelectedType;
      tm.m_pLastSelectedType = nullptr;
    }
  }

  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  WAddObjectCommand cmdAdd;
  cmdAdd.m_pType = WGetStaticRTTI<WGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  WUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty() || !bAttachToParent)
  {
    cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    if (!bAttachToParent)
    {
      const WUuid activeParent = GetRedirectedGameObjectDoc()->GetActiveParent();

      // the object may not exist anymore
      if (auto pParentObj = GetObjectManager()->GetObject(activeParent))
      {
        cmdAdd.m_Parent = activeParent;
      }
    }

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid = WUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    cmdAdd.m_Parent = Sel[0]->GetGuid();
    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  if (!bAttachToParent && bAtPickedPosition && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
  {
    WVec3 position = ctxt.m_pLastPickingResult->m_vPickedPosition;

    WSnapProvider::SnapTranslation(position);

    WSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue = position;
    cmdSet.m_Object = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    if (auto pParentObj = GetObjectManager()->GetObject(cmdAdd.m_Parent))
    {
      const WTransform tParent = GetGlobalTransform(pParentObj);
      const WTransform tRel = WTransform::MakeLocalTransform(tParent, WTransform(position, WQuat::MakeIdentity()));

      cmdSet.m_NewValue = tRel.m_vPosition;
    }

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  // Add a dummy shape icon component, which enables picking
  {
    WAddObjectCommand cmdAdd;
    cmdAdd.m_pType = pComponentType;
    cmdAdd.m_sParentProperty = "Components";
    cmdAdd.m_Index = -1;
    cmdAdd.m_Parent = NewNode;

    auto res = history->AddCommand(cmdAdd);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return WStatus(W_SUCCESS);
}

void WSceneDocument::DuplicateSelection()
{
  WMap<WUuid, WUuid> parents;

  WAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  WStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", WConversionUtils::ToString(it.Key(), tmp1), WConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  WContiguousMemoryStreamStorage streamStorage;
  WMemoryStreamWriter memoryWriter(&streamStorage);

  WAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  WDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;

  // When exactly one object is selected, place the duplicate right after the original in the parent's children list.
  if (parents.GetCount() == 1)
  {
    WTempHybridArray<WSelectionEntry, 4> topLevelSel;
    GetSelectionManager()->GetTopLevelSelection(topLevelSel);

    if (topLevelSel.GetCount() == 1)
    {
      const WVariant idx = topLevelSel[0].m_pObject->GetPropertyIndex();
      if (idx.IsValid())
        cmd.m_iInsertIndex = idx.ConvertTo<WInt32>() + 1;
    }
  }

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Selection");

  if (history->AddCommand(cmd).Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void WSceneDocument::ShowOrHideSelectedObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
      continue;

    ApplyRecursive(pItem, [this, bHide](const WDocumentObject* pObj)
      {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData->EndModifyMetaData(0); });
  }
}

void WSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

void WSceneDocument::SetGameMode(GameMode::Enum mode)
{
  if (m_GameMode == mode)
    return;

  // store settings of recently active mode
  m_GameModeData[m_GameMode] = m_CurrentMode;

  m_GameMode = mode;
  SetPauseSimulation(false);

  switch (m_GameMode)
  {
    case GameMode::Off:
      ShowDocumentStatus("Game Mode: Off");
      break;
    case GameMode::Simulate:
      ShowDocumentStatus("Game Mode: Simulate");
      break;
    case GameMode::Play:
      ShowDocumentStatus("Game Mode: Play");
      break;
  }

  SetRenderSelectionOverlay(m_GameModeData[m_GameMode].m_bRenderSelectionOverlay);
  SetRenderShapeIcons(m_GameModeData[m_GameMode].m_bRenderShapeIcons);
  SetRenderVisualizers(m_GameModeData[m_GameMode].m_bRenderVisualizers);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    SendGameWorldToEngine();
  }

  WGameObjectEvent e;
  e.m_Type = WGameObjectEvent::Type::GameModeChanged;
  m_GameObjectEvents.Broadcast(e);

  ScheduleSendObjectSelection();
}

WStatus WSceneDocument::CreatePrefabDocumentFromSelection(WStringView sFile, const WRTTI* pRootType, WDelegate<void(WAbstractObjectNode*)> adjustGraphNodeCB /* = {} */, WDelegate<void(WDocumentObject*)> adjustNewNodesCB /* = {} */, WDelegate<void(WAbstractObjectGraph& graph, WDynamicArray<WAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB /* = {} */)
{
  WTempHybridArray<WSelectionEntry, 32> Selection;
  GetSelectionManager()->GetTopLevelSelectionOfType(pRootType, Selection);

  if (Selection.IsEmpty())
    return WStatus("To create a prefab, the selection must not be empty");

  const WTransform tReference = QueryLocalTransform(Selection.PeekBack().m_pObject);

  WVariantArray varChildren;

  auto centerNodes = [tReference, &varChildren](WAbstractObjectNode* pGraphNode)
  {
    if (auto pPosition = pGraphNode->FindProperty("LocalPosition"))
    {
      WVec3 pos = pPosition->m_Value.ConvertTo<WVec3>();
      pos -= tReference.m_vPosition;

      pGraphNode->ChangeProperty("LocalPosition", pos);
    }

    if (auto pRotation = pGraphNode->FindProperty("LocalRotation"))
    {
      WQuat rot = pRotation->m_Value.ConvertTo<WQuat>();
      rot = tReference.m_qRotation.GetInverse() * rot;

      pGraphNode->ChangeProperty("LocalRotation", rot);
    }

    varChildren.PushBack(pGraphNode->GetGuid());
  };

  auto adjustResult = [tReference, this](WDocumentObject* pObject)
  {
    const WTransform tOld = QueryLocalTransform(pObject);

    WSetObjectPropertyCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    cmd.m_sProperty = "LocalPosition";
    cmd.m_NewValue = tOld.m_vPosition + tReference.m_vPosition;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();

    cmd.m_sProperty = "LocalRotation";
    cmd.m_NewValue = tReference.m_qRotation * tOld.m_qRotation;
    GetCommandHistory()->AddCommand(cmd).AssertSuccess();
  };

  auto finalizeGraph = [this, &varChildren](WAbstractObjectGraph& ref_graph, WDynamicArray<WAbstractObjectNode*>& ref_graphRootNodes)
  {
    if (ref_graphRootNodes.GetCount() == 1)
    {
      ref_graphRootNodes[0]->ChangeProperty("Name", "<Prefab-Root>");
    }
    else
    {
      const WRTTI* pRtti = WGetStaticRTTI<WGameObject>();

      WAbstractObjectNode* pRoot = ref_graph.AddNode(WUuid::MakeUuid(), pRtti->GetTypeName(), pRtti->GetTypeVersion());
      pRoot->AddProperty("Name", "<Prefab-Root>");
      pRoot->AddProperty("Children", varChildren);

      ref_graphRootNodes.Clear();
      ref_graphRootNodes.PushBack(pRoot);
    }
  };

  if (!adjustGraphNodeCB.IsValid())
    adjustGraphNodeCB = centerNodes;
  if (!adjustNewNodesCB.IsValid())
    adjustNewNodesCB = adjustResult;
  if (!finalizeGraphCB.IsValid())
    finalizeGraphCB = finalizeGraph;

  return SUPER::CreatePrefabDocumentFromSelection(sFile, pRootType, adjustGraphNodeCB, adjustNewNodesCB, finalizeGraphCB);
}

bool WSceneDocument::CanEngineProcessBeRestarted() const
{
  return m_GameMode == GameMode::Off;
}

void WSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    WGameObjectDocumentEvent e;
    e.m_Type = WGameObjectDocumentEvent::Type::GameMode_StartingSimulate;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  SetGameMode(GameMode::Simulate);
}


void WSceneDocument::TriggerGameModePlay(bool bUsePickedPositionAsStart)
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    WGameObjectDocumentEvent e;
    e.m_Type = WGameObjectDocumentEvent::Type::GameMode_StartingPlay;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  UpdateObjectDebugTargets();

  // attempt to start PTG
  // do not change state here
  {
    WGameModeMsgToEngine msg;
    msg.m_bEnablePTG = true;
    msg.m_bUseStartPosition = false;

    if (bUsePickedPositionAsStart)
    {
      const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

      if (ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastHoveredViewWidget->GetDocumentWindow()->GetDocument() == this)
      {
        msg.m_bUseStartPosition = true;
        msg.m_vStartPosition = ctxt.m_pLastPickingResult->m_vPickedPosition;

        WVec3 vPickDir = ctxt.m_pLastPickingResult->m_vPickedPosition - ctxt.m_pLastPickingResult->m_vPickingRayStart;
        vPickDir.z = 0;
        vPickDir.NormalizeIfNotZero(WVec3(1, 0, 0)).IgnoreResult();

        msg.m_vStartDirection = vPickDir;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


bool WSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return false;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg
    {
      WGameModeMsgToEngine msg;
      msg.m_bEnablePTG = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  {
    WGameObjectDocumentEvent e;
    e.m_Type = WGameObjectDocumentEvent::Type::GameMode_Stopped;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  return true;
}

void WSceneDocument::StepSimulation()
{
  SetStepSimulation(true);
}

void WSceneDocument::PauseSimulation()
{
  SetPauseSimulation(true);
}

void WSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const WDocumentObject* pObj)
    {
    // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    // return;

    WUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = WDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData->EndModifyMetaData(uiFlags); });
}
void WSceneDocument::GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/WEditor.WAbstractGraph");
}

bool WSceneDocument::CopySelectedObjects(WAbstractObjectGraph& ref_graph, WStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/WEditor.WAbstractGraph";
  return CopySelectedObjects(ref_graph, nullptr);
}

bool WSceneDocument::CopySelectedObjects(WAbstractObjectGraph& ref_graph, WMap<WUuid, WUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  WTempHybridArray<WSelectionEntry, 64> selection;
  GetSelectionManager()->GetTopLevelSelection(selection);

  WDocumentObjectConverterWriter writer(&ref_graph, GetObjectManager());

  // objects are required to be named root but this is not enforced or obvious by the interface.
  for (WUInt32 i = 0; i < selection.GetCount(); i++)
  {
    const auto& item = selection[i];
    WAbstractObjectNode* pNode = writer.AddObjectToGraph(item.m_pObject, "root");
    pNode->AddProperty("__GlobalTransform", GetGlobalTransform(item.m_pObject));
    pNode->AddProperty("__Order", i);
    pNode->AddProperty("__SelectionOrder", item.m_uiSelectionOrder);
  }

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (const auto& item : selection)
    {
      (*out_pParents)[item.m_pObject->GetGuid()] = item.m_pObject->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(ref_graph);

  return true;
}

bool WSceneDocument::PasteAt(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, const WVec3& vPasteAt)
{
  WTransform refTransform = WTransform::MakeIdentity();
  WUInt32 uiHighestSelectionOrder = 0;

  WTempHybridArray<WTransform, 16> globalTransforms;
  globalTransforms.SetCount(info.GetCount(), WTransform::MakeIdentity());

  for (WUInt32 i = 0; i < info.GetCount(); ++i)
  {
    const PasteInfo& pi = info[i];

    if (pi.m_pObject->GetTypeAccessor().GetType() != WGetStaticRTTI<WGameObject>())
      return false;

    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        globalTransforms[i] = pProperty->m_Value.Get<WTransform>();

        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          // find the last selected element, and use it as the reference point for the paste position

          const WUInt32 uiSelOrder = pProperty->m_Value.ConvertTo<WUInt32>();
          if (uiSelOrder >= uiHighestSelectionOrder)
          {
            uiHighestSelectionOrder = uiSelOrder;
            refTransform = globalTransforms[i];
          }
        }
      }
    }
  }

  for (WUInt32 i = 0; i < info.GetCount(); ++i)
  {
    const PasteInfo& pi = info[i];

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }

    WTransform tNew = globalTransforms[i];
    tNew.m_vPosition -= refTransform.m_vPosition;
    tNew.m_vPosition += vPasteAt;

    SetGlobalTransform(pi.m_pObject, tNew, TransformationChanges::All);
  }

  return true;
}

bool WSceneDocument::PasteAtOrignalPosition(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }

    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        if (pProperty->m_Value.IsA<WTransform>())
        {
          SetGlobalTransform(pi.m_pObject, pProperty->m_Value.Get<WTransform>(), TransformationChanges::All);
        }
      }
    }
  }

  return true;
}

bool WSceneDocument::Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType)
{
  const auto& ctxt = WQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    WVec3 pos = ctxt.m_pLastPickingResult->m_vPickedPosition;
    WSnapProvider::SnapTranslation(pos);

    if (!PasteAt(info, objectGraph, pos))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info, objectGraph))
      return false;
  }

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    WDeque<const WDocumentObject*> NewSelection;
    NewSelection.SetCount(info.GetCount());

    for (WUInt32 i = 0; i < info.GetCount(); ++i)
    {
      const PasteInfo& pi = info[i];

      WUInt32 order = i;
      if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
      {
        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          order = pProperty->m_Value.ConvertTo<WUInt32>();
        }
      }

      NewSelection[order] = pi.m_pObject;
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool WSceneDocument::DuplicateSelectedObjects(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info, objectGraph))
    return false;

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  if (bSetSelected)
  {
    auto pSelMan = GetSelectionManager();

    WDeque<const WDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

void WSceneDocument::EnsureSettingsObjectExist()
{
  // Settings object was changed to have a base class and each document type has a different implementation.
  const WRTTI* pSettingsType = nullptr;
  switch (m_DocumentType)
  {
    case WSceneDocument::DocumentType::Scene:
      pSettingsType = WGetStaticRTTI<WSceneDocumentSettings>();
      break;
    case WSceneDocument::DocumentType::Prefab:
      pSettingsType = WGetStaticRTTI<WPrefabDocumentSettings>();
      break;
    case WSceneDocument::DocumentType::Layer:
      pSettingsType = WGetStaticRTTI<WLayerDocumentSettings>();
      break;
  }

  auto pRoot = GetObjectManager()->GetRootObject();
  // Use the WObjectDirectAccessor instead of calling GetObjectAccessor because we do not want
  // undo ops for this operation.
  WObjectDirectAccessor accessor(GetObjectManager());
  WVariant value;
  W_VERIFY(accessor.WObjectAccessorBase::GetValueByName(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  WUuid id = value.Get<WUuid>();
  if (!id.IsValid())
  {
    W_VERIFY(accessor.WObjectAccessorBase::AddObjectByName(pRoot, "Settings", WVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
  }
  else
  {
    WDocumentObject* pSettings = GetObjectManager()->GetObject(id);
    W_VERIFY(pSettings, "Document corrupt, root references a non-existing object");
    if (pSettings->GetType() != pSettingsType)
    {
      accessor.RemoveObject(pSettings).AssertSuccess();
      GetObjectManager()->DestroyObject(pSettings);
      W_VERIFY(accessor.WObjectAccessorBase::AddObjectByName(pRoot, "Settings", WVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
    }
  }
}

const WDocumentObject* WSceneDocument::GetSettingsObject() const
{
  auto pRoot = GetObjectManager()->GetRootObject();
  WVariant value;
  W_VERIFY(GetObjectAccessor()->GetValueByName(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  WUuid id = value.Get<WUuid>();
  return GetObjectManager()->GetObject(id);
}

const WSceneDocumentSettingsBase* WSceneDocument::GetSettingsBase() const
{
  return static_cast<const WSceneDocumentSettingsBase*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));
}

WStatus WSceneDocument::CreateExposedProperty(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index, WExposedSceneProperty& out_key) const
{
  WTempHybridArray<WVariant, 2> path;
  if (index.IsValid())
    path.PushBack(index);

  pAccessor = pAccessor->ResolveProxy(pObject, pType, pProperty, path);

  const WDocumentObject* pNodeComponent = WObjectPropertyPath::FindParentNodeComponent(pObject);
  if (!pObject)
    return WStatus("No parent node or component found.");

  WObjectPropertyPathContext context = {pNodeComponent, pAccessor, "Children"};
  WVariant firstIndex;
  if (!path.IsEmpty())
    firstIndex = path[0];

  WPropertyReference propertyRef = {pObject->GetGuid(), pProperty, firstIndex};
  WStringBuilder sPropertyPath;
  WStatus res = WObjectPropertyPath::CreatePropertyPath(context, propertyRef, sPropertyPath);
  if (res.Failed())
    return res;

  if (path.GetCount() > 1)
    WObjectPropertyPath::AppendSubIndices(sPropertyPath, path.GetArrayPtr().GetSubArray(1));

  out_key.m_Object = pNodeComponent->GetGuid();
  out_key.m_sPropertyPath = sPropertyPath;
  return WStatus(W_SUCCESS);
}

WStatus WSceneDocument::AddExposedParameter(const char* szName, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index)
{
  if (m_DocumentType != DocumentType::Prefab)
    return WStatus("Exposed parameters are only supported in prefab documents.");

  if (FindExposedParameter(pAccessor, pObject, pType, pProperty, index) != -1)
    return WStatus("Exposed parameter already exists.");

  WExposedSceneProperty key;
  WStatus res = CreateExposedProperty(pAccessor, pObject, pType, pProperty, index, key);
  if (res.Failed())
    return res;

  WUuid id;
  res = GetObjectAccessor()->AddObjectByName(GetSettingsObject(), "ExposedProperties", -1, WGetStaticRTTI<WExposedSceneProperty>(), id);
  if (res.Failed())
    return res;
  const WDocumentObject* pParam = GetObjectManager()->GetObject(id);
  GetObjectAccessor()->SetValueByName(pParam, "Name", szName).LogFailure();
  GetObjectAccessor()->SetValueByName(pParam, "Object", key.m_Object).LogFailure();
  GetObjectAccessor()->SetValueByName(pParam, "PropertyPath", WVariant(key.m_sPropertyPath)).LogFailure();
  return WStatus(W_SUCCESS);
}

WInt32 WSceneDocument::FindExposedParameter(WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WRTTI* pType, const WAbstractProperty* pProperty, WVariant index)
{
  W_ASSERT_DEV(m_DocumentType == DocumentType::Prefab, "Exposed properties are only supported in prefab documents.");

  WExposedSceneProperty key;
  WStatus res = CreateExposedProperty(pAccessor, pObject, pType, pProperty, index, key);
  if (res.Failed())
    return -1;

  const WPrefabDocumentSettings* settings = GetSettings<WPrefabDocumentSettings>();
  for (WUInt32 i = 0; i < settings->m_ExposedProperties.GetCount(); i++)
  {
    const auto& param = settings->m_ExposedProperties[i];
    if (param.m_Object == key.m_Object && param.m_sPropertyPath == key.m_sPropertyPath)
      return (WInt32)i;
  }
  return -1;
}

WStatus WSceneDocument::RemoveExposedParameter(WInt32 iIndex)
{
  WVariant value;
  auto res = GetObjectAccessor()->GetValueByName(GetSettingsObject(), "ExposedProperties", value, iIndex);
  if (res.Failed())
    return res;

  WUuid id = value.Get<WUuid>();
  return GetObjectAccessor()->RemoveObject(GetObjectManager()->GetObject(id));
}


void WSceneDocument::StoreFavoriteCamera(WUInt8 uiSlot)
{
  W_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  WQuadViewPreferencesUser* pPreferences = WPreferences::QueryPreferences<WQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = WQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    const auto& camera = pView->m_pViewConfig->m_Camera;

    cam.m_PerspectiveMode = pView->m_pViewConfig->m_Perspective;
    cam.m_vCamPos = camera.GetCenterPosition();
    cam.m_vCamDir = camera.GetCenterDirForwards();
    cam.m_vCamUp = camera.GetCenterDirUp();

    // make sure the data gets saved
    pPreferences->TriggerPreferencesChangedEvent();
  }
}

void WSceneDocument::RestoreFavoriteCamera(WUInt8 uiSlot)
{
  W_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  WQuadViewPreferencesUser* pPreferences = WPreferences::QueryPreferences<WQuadViewPreferencesUser>(this);
  auto& cam = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = WQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return;

  WVec3 vCamPos = cam.m_vCamPos;
  WVec3 vCamDir = cam.m_vCamDir;
  WVec3 vCamUp = cam.m_vCamUp;

  // if the projection mode of the view is orthographic, ignore the direction of the stored favorite camera
  // if we apply a favorite that was saved in an orthographic view, and we apply it to a perspective view,
  // we want to ignore one of the axis, as the respective orthographic position can be arbitrary
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case WSceneViewPerspective::Orthogonal_Front:
    case WSceneViewPerspective::Orthogonal_Right:
    case WSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case WSceneViewPerspective::Perspective:
    {
      const WVec3 vOldPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();

      switch (cam.m_PerspectiveMode)
      {
        case WSceneViewPerspective::Orthogonal_Front:
          vCamPos.x = vOldPos.x;
          break;
        case WSceneViewPerspective::Orthogonal_Right:
          vCamPos.y = vOldPos.y;
          break;
        case WSceneViewPerspective::Orthogonal_Top:
          vCamPos.z = vOldPos.z;
          break;
        case WSceneViewPerspective::Perspective:
          break;
      }

      break;
    }
  }

  pView->InterpolateCameraTo(vCamPos, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp);
}

WResult WSceneDocument::JumpToLevelCamera(WUInt8 uiSlot, bool bImmediate)
{
  W_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = WQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return W_FAILURE;

  auto* pObjMan = GetObjectManager();

  WTempHybridArray<WDocumentObject*, 8> stack;
  stack.PushBack(pObjMan->GetRootObject());

  const WRTTI* pCamType = WGetStaticRTTI<WCameraComponent>();
  const WDocumentObject* pCamObj = nullptr;

  while (!stack.IsEmpty())
  {
    const WDocumentObject* pObj = stack.PeekBack();
    stack.PopBack();

    stack.PushBackRange(pObj->GetChildren());

    if (pObj->GetType() == pCamType)
    {
      WInt32 iShortcut = pObj->GetTypeAccessor().GetValue("EditorShortcut").ConvertTo<WInt32>();

      if (iShortcut == uiSlot)
      {
        pCamObj = pObj->GetParent();
        break;
      }
    }
  }

  if (pCamObj == nullptr)
    return W_FAILURE;

  const WTransform tCam = GetGlobalTransform(pCamObj);

  WVec3 vCamDir = tCam.m_qRotation * WVec3(1, 0, 0);
  WVec3 vCamUp = tCam.m_qRotation * WVec3(0, 0, 1);

  // if the projection mode of the view is orthographic, ignore the direction of the level camera
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case WSceneViewPerspective::Orthogonal_Front:
    case WSceneViewPerspective::Orthogonal_Right:
    case WSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case WSceneViewPerspective::Perspective:
      break;
  }

  pView->InterpolateCameraTo(tCam.m_vPosition, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp, bImmediate);

  return W_SUCCESS;
}

WResult WSceneDocument::CreateLevelCamera(WUInt8 uiSlot)
{
  W_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = WQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return W_FAILURE;

  if (pView->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
    return W_FAILURE;

  const auto* pRootObj = GetObjectManager()->GetRootObject();

  const WVec3 vPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();
  const WVec3 vDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards().GetNormalized();
  const WVec3 vUp = pView->m_pViewConfig->m_Camera.GetCenterDirUp().GetNormalized();

  auto* pAccessor = GetObjectAccessor();
  pAccessor->StartTransaction("Create Level Camera");

  WUuid camObjGuid;
  if (pAccessor->AddObjectByName(pRootObj, "Children", -1, WGetStaticRTTI<WGameObject>(), camObjGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return W_FAILURE;
  }

  WMat3 mRot;
  mRot.SetColumn(0, vDir);
  mRot.SetColumn(1, vUp.CrossRH(vDir).GetNormalized());
  mRot.SetColumn(2, vUp);
  WQuat qRot;
  qRot = WQuat::MakeFromMat3(mRot);
  qRot.Normalize();

  SetGlobalTransform(pAccessor->GetObject(camObjGuid), WTransform(vPos, qRot), TransformationChanges::Translation | TransformationChanges::Rotation);

  WUuid camCompGuid;
  if (pAccessor->AddObjectByName(pAccessor->GetObject(camObjGuid), "Components", -1, WGetStaticRTTI<WCameraComponent>(), camCompGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return W_FAILURE;
  }

  if (pAccessor->SetValueByName(pAccessor->GetObject(camCompGuid), "EditorShortcut", uiSlot).Failed())
  {
    pAccessor->CancelTransaction();
    return W_FAILURE;
  }

  pAccessor->FinishTransaction();
  return W_SUCCESS;
}

void WSceneDocument::DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & WDocumentObjectMetaData::HiddenFlag) != 0)
  {
    WObjectTagMsgToEngine msg;
    msg.m_bSetTag = e.m_pValue->m_bHidden;
    msg.m_sTag = "EditorHidden";
    msg.m_bApplyOnAllChildren = true;

    SendObjectMsg(GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void WSceneDocument::EngineConnectionEventHandler(const WEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case WEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    case WEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case WEditorEngineProcessConnection::Event::Type::ProcessStarted:
      SetGameMode(GameMode::Off);
      break;

    default:
      break;
  }
}


void WSceneDocument::ToolsProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectConfigChanged:
    {
      // we are lazy and just refresh the current selection here
      // that ensures that ui elements will rebuild their content

      GetSelectionManager()->RefreshSelection();
    }
    break;

    default:
      break;
  }
}

void WSceneDocument::HandleGameModeMsg(const WGameModeMsgToEditor* pMsg)
{
  if (m_GameMode == GameMode::Simulate)
  {
    if (pMsg->m_bRunningPTG)
    {
      m_GameMode = GameMode::Off;
      WLog::Warning("Incorrect state change from 'simulate' to 'play-the-game'");
    }
    else
    {
      // probably the message just arrived late ?
      return;
    }
  }

  if (m_GameMode == GameMode::Off || m_GameMode == GameMode::Play)
  {
    SetGameMode(pMsg->m_bRunningPTG ? GameMode::Play : GameMode::Off);
    return;
  }

  W_REPORT_FAILURE("Unreachable Code reached.");
}

void WSceneDocument::HandleObjectStateFromEngineMsg(const WPushObjectStateMsgToEditor* pMsg)
{
  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Pull Object State");

  for (const auto& state : pMsg->m_ObjectStates)
  {
    auto pObject = GetObjectManager()->GetObject(state.m_ObjectGuid);

    if (pObject)
    {
      SetGlobalTransform(pObject, WTransform(state.m_vPosition, state.m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }

  pHistory->FinishTransaction();
}

void WSceneDocument::SendObjectMsg(const WDocumentObject* pObj, WObjectTagMsgToEngine* pMsg)
{
  // if WObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}

void WSceneDocument::SendObjectMsgRecursive(const WDocumentObject* pObj, WObjectTagMsgToEngine* pMsg)
{
  // if WObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void WSceneDocument::GatherObjectsOfType(WDocumentObject* pRoot, WGatherObjectsOfTypeMsgInterDoc* pMsg) const
{
  if (pRoot->GetType() == pMsg->m_pType)
  {
    WStringBuilder sFullPath;
    GenerateFullDisplayName(pRoot, sFullPath);

    auto& res = pMsg->m_Results.ExpandAndGetRef();
    res.m_ObjectGuid = pRoot->GetGuid();
    res.m_pDocument = this;
    res.m_sDisplayName = sFullPath;
  }

  for (auto pChild : pRoot->GetChildren())
  {
    GatherObjectsOfType(pChild, pMsg);
  }
}

void WSceneDocument::SelectionManagerEventHandler(const WSelectionManagerEvent& e)
{
  if (!m_bStoreSelectionChange)
    return;

  if (m_iAllowSelectionChanges != -1)
  {
    if (m_iAllowSelectionChanges == 0)
      m_SelectionStack.PopBack();

    --m_iAllowSelectionChanges;
  }

  switch (e.m_Type)
  {
    case WSelectionManagerEvent::Type::ObjectAdded:
    case WSelectionManagerEvent::Type::ObjectRemoved:
    case WSelectionManagerEvent::Type::SelectionSet:
    case WSelectionManagerEvent::Type::SelectionCleared: // empty selections are important to keep, for layer changes to be undoable
    {
      const auto& curSel = GetSelectionManager()->GetSelection();

      auto& sel = m_SelectionStack.ExpandAndGetRef();

      sel.m_documentGuid = GetRedirectedGameObjectDoc()->GetGuid();

      sel.m_Objects.SetCountUninitialized(curSel.GetCount());

      for (WUInt32 i = 0; i < curSel.GetCount(); ++i)
      {
        sel.m_Objects[i] = curSel[i]->GetGuid();
      }

      // discard duplicate selection changes (but keep empty selections)
      if (m_SelectionStack.GetCount() > 1)
      {
        const auto& prev = m_SelectionStack[m_SelectionStack.GetCount() - 2];

        if (prev.m_Objects == sel.m_Objects && prev.m_documentGuid == sel.m_documentGuid)
        {
          m_SelectionStack.PopBack();
        }
      }

      if (m_SelectionStack.GetCount() > 16)
      {
        m_SelectionStack.PopFront();
      }

      break;
    }

    default:
      break;
  }
}

bool WSceneDocument::CanUndoSelection() const
{
  return m_SelectionStack.GetCount() > 1;
}

void WSceneDocument::UndoSelection()
{
  if (m_SelectionStack.IsEmpty())
    return;

  if (m_SelectionStack.GetCount() > 1)
    m_SelectionStack.PopBack();

  auto& back = m_SelectionStack.PeekBack();

  m_bStoreSelectionChange = false;
  W_SCOPE_EXIT(m_bStoreSelectionChange = true);

  auto* pDoc = WDocumentManager::GetDocumentByGuid(back.m_documentGuid);
  if (pDoc == nullptr)
    return;

  auto pObjMan = pDoc->GetObjectManager();

  WDeque<const WDocumentObject*> newSel;
  for (const WUuid& guid : back.m_Objects)
  {
    if (auto pDoc = pObjMan->GetObject(guid))
    {
      newSel.PushBack(pDoc);
    }
  }

  GetSelectionManager()->SetSelection(newSel);
}

void WSceneDocument::OnInterDocumentMessage(WReflectedClass* pMessage, WDocument* pSender)
{
  // #TODO needs to be overwritten by Scene2
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<WGatherObjectsOfTypeMsgInterDoc>())
  {
    GatherObjectsOfType(GetObjectManager()->GetRootObject(), static_cast<WGatherObjectsOfTypeMsgInterDoc*>(pMessage));
  }
}

WStatus WSceneDocument::RequestExportScene(const char* szTargetFile, const WAssetFileHeader& header)
{
  if (GetGameMode() != GameMode::Off)
    return WStatus("Cannot export while the scene is simulating");

  W_SUCCEED_OR_RETURN(SaveDocument());

  W_SUCCEED_OR_RETURN(WaitForEngineStatusLoaded());

  // Ensure child order information is synced to the engine before export.
  // When a scene is transformed in the background (not opened in a window), the
  // WDocumentOpenResponseMsgToEditor that normally triggers SyncAllChildOrders() is never
  // received, so components with WSyncChildOrderAttribute (e.g. WSplineComponent) would
  // export with incorrect child order data.
  SyncAllChildOrders();

  const WStatus status = WAssetDocument::RemoteExport(header, szTargetFile);

  // make sure the world is reset
  SendGameWorldToEngine();

  return status;
}

void WSceneDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // scenes do not have exposed parameters
  if (!IsPrefab())
    return;

  WExposedParameters* pExposedParams = W_DEFAULT_NEW(WExposedParameters);

  if (m_DocumentType == DocumentType::Prefab)
  {
    WSet<WString> alreadyExposed;

    auto pSettings = GetSettings<WPrefabDocumentSettings>();
    for (auto prop : pSettings->m_ExposedProperties)
    {
      auto pRootObject = GetObjectManager()->GetObject(prop.m_Object);
      if (!pRootObject)
      {
        WLog::Warning("The exposed scene property '{0}' does not point to a valid object and is skipped.", prop.m_sName);
        continue;
      }

      WObjectPropertyPathContext context = {pRootObject, GetObjectAccessor(), "Children"};

      WPropertyReference key;
      auto res = WObjectPropertyPath::ResolvePropertyPath(context, prop.m_sPropertyPath, key);
      if (res.Failed())
      {
        WLog::Warning("The exposed scene property '{0}' can no longer be resolved and is skipped.", prop.m_sName);
        continue;
      }
      WVariant value;

      const WExposedParameter* pSourceParameter = nullptr;
      auto pLeafObject = GetObjectManager()->GetObject(key.m_Object);
      if (const WExposedParametersAttribute* pAttrib = key.m_pProperty->GetAttributeByType<WExposedParametersAttribute>())
      {
        // If the target of the exposed parameter is yet another exposed parameter, we need to do the following:
        // A: Get the default value from via an WExposedParameterCommandAccessor. This ensures that in case the target does not actually exist (because it was not overwritten in this template instance) we get the default parameter of the exposed param instead.
        // B: Replace the property type of the exposed parameter (which will always be an WVariant inside the WVariantDictionary) with the property type the exposed parameter actually points to.
        const WAbstractProperty* pParameterSourceProp = pLeafObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
        W_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), pLeafObject->GetType()->GetTypeName());

        WExposedParameterCommandAccessor proxy(context.m_pAccessor, key.m_pProperty, pParameterSourceProp);
        res = proxy.GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
        if (key.m_Index.IsA<WString>())
        {
          pSourceParameter = proxy.GetExposedParam(pLeafObject, key.m_Index.Get<WString>());
        }
      }
      else
      {
        res = context.m_pAccessor->GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      W_ASSERT_DEBUG(res.Succeeded(), "ResolvePropertyPath succeeded so GetValue should too");

      // do not show the same parameter twice, even if they have different types, as the UI doesn't handle that case properly
      // TODO: we should prevent users from using the same name for differently typed parameters
      // don't do this earlier, we do want to validate each exposed property with the code above
      if (alreadyExposed.Contains(prop.m_sName))
        continue;

      alreadyExposed.Insert(prop.m_sName);

      WExposedParameter* param = W_DEFAULT_NEW(WExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = prop.m_sName;
      param->m_DefaultValue = value;
      if (pSourceParameter)
      {
        // Copy properties of exposed parameter that we are exposing one level deeper.
        param->m_sType = pSourceParameter->m_sType;
        for (auto attrib : pSourceParameter->m_Attributes)
        {
          param->m_Attributes.PushBack(WReflectionSerializer::Clone(attrib));
        }
      }
      else
      {
        param->m_sType = key.m_pProperty->GetSpecificType()->GetTypeName();
        for (auto attrib : key.m_pProperty->GetAttributes())
        {
          param->m_Attributes.PushBack(WReflectionSerializer::Clone(attrib));
        }
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

WTransformStatus WSceneDocument::ExportScene(bool bCreateThumbnail)
{
  if (GetUnknownObjectTypeInstances() > 0)
  {
    return WTransformStatus("Can't export scene/prefab when it contains unknown object types.");
  }

  // #TODO export layers
  auto saveres = SaveDocument();

  if (saveres.Failed())
    return saveres;

  WTransformStatus res;

  if (bCreateThumbnail)
  {
    // this is needed to generate a scene thumbnail, however that has a larger overhead (1 sec or so)
    res = WAssetCurator::GetSingleton()->TransformAsset(GetGuid(), WTransformFlags::ForceTransform | WTransformFlags::TriggeredManually);
  }
  else
  {
    res = TransformAsset(WTransformFlags::ForceTransform | WTransformFlags::TriggeredManually);
  }

  if (res.Failed())
    WLog::Error(res.m_sMessage);
  else
    WLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}

void WSceneDocument::ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode, const WMat3& mTransform)
{
  WExportSceneGeometryMsgToEngine msg;
  msg.m_sOutputFile = szFile;
  msg.m_bSelectionOnly = bOnlySelection;
  msg.m_iExtractionMode = iExtractionMode;
  msg.m_Transform = mTransform;

  SendMessageToEngine(&msg);

  WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Geometry exported to '{0}'", szFile), WTime::MakeFromSeconds(5.0f));
}

void WSceneDocument::HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg)
{
  WGameObjectDocument::HandleEngineMessage(pMsg);

  if (const WGameModeMsgToEditor* msg = WDynamicCast<const WGameModeMsgToEditor*>(pMsg))
  {
    HandleGameModeMsg(msg);
    return;
  }

  if (const WDocumentOpenResponseMsgToEditor* msg = WDynamicCast<const WDocumentOpenResponseMsgToEditor*>(pMsg))
  {
    SyncObjectHiddenState();
    SyncAllChildOrders();
  }

  if (const WPushObjectStateMsgToEditor* msg = WDynamicCast<const WPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg(msg);
  }
}

WTransformStatus WSceneDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  if (m_DocumentType == DocumentType::Prefab)
  {
    const WPrefabDocumentSettings* pSettings = GetSettings<WPrefabDocumentSettings>();

    if (GetEditorEngineConnection() != nullptr)
    {
      WExposedDocumentObjectPropertiesMsgToEngine msg;
      msg.m_Properties = pSettings->m_ExposedProperties;

      SendMessageToEngine(&msg);
    }
  }
  return RequestExportScene(szTargetFile, AssetHeader);
}


WTransformStatus WSceneDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  W_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return WStatus(W_FAILURE);
}


WTransformStatus WSceneDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  WStatus status = WAssetDocument::RemoteCreateThumbnail(ThumbnailInfo, {});

  // if we were to do this BEFORE making the screenshot, the scene would be killed, but not immediately restored
  // and the screenshot would end up empty
  // instead the engine side ensures simulation is stopped and makes a screenshot of whatever state is visible
  // but then the editor and engine state are out of sync, so AFTER the screenshot is done,
  // we ensure to also stop simulation on the editor side
  StopGameMode();

  return status;
}

void WSceneDocument::SyncObjectHiddenState()
{
  // #TODO Scene2 handling
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void WSceneDocument::SyncObjectHiddenState(WDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData->EndReadMetaData();

  WObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void WSceneDocument::UpdateObjectDebugTargets()
{
  WGatherObjectsForDebugVisMsgInterDoc msg;
  BroadcastInterDocumentMessage(&msg, this);

  {
    WObjectsForDebugVisMsgToEngine msgToEngine;
    msgToEngine.m_Objects.SetCountUninitialized(sizeof(WUuid) * msg.m_Objects.GetCount());

    WMemoryUtils::Copy<WUInt8>(msgToEngine.m_Objects.GetData(), reinterpret_cast<WUInt8*>(msg.m_Objects.GetData()), msgToEngine.m_Objects.GetCount());

    GetEditorEngineConnection()->SendMessage(&msgToEngine);
  }
}
