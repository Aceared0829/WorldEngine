#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSelectionAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WSelectionActions::s_hGroupSelectedItems;
WActionDescriptorHandle WSelectionActions::s_hCreateEmptyChildObject;
WActionDescriptorHandle WSelectionActions::s_hCreateEmptyObjectAtPosition;
WActionDescriptorHandle WSelectionActions::s_hHideSelectedObjects;
WActionDescriptorHandle WSelectionActions::s_hHideUnselectedObjects;
WActionDescriptorHandle WSelectionActions::s_hShowHiddenObjects;
WActionDescriptorHandle WSelectionActions::s_hPrefabMenu;
WActionDescriptorHandle WSelectionActions::s_hCreatePrefab;
WActionDescriptorHandle WSelectionActions::s_hRevertPrefab;
WActionDescriptorHandle WSelectionActions::s_hUnlinkFromPrefab;
WActionDescriptorHandle WSelectionActions::s_hOpenPrefabDocument;
WActionDescriptorHandle WSelectionActions::s_hDuplicateSpecial;
WActionDescriptorHandle WSelectionActions::s_hDeltaTransform;
WActionDescriptorHandle WSelectionActions::s_hSnapObjectToCamera;
WActionDescriptorHandle WSelectionActions::s_hAttachToObject;
WActionDescriptorHandle WSelectionActions::s_hDetachFromParent;
WActionDescriptorHandle WSelectionActions::s_hConvertToEnginePrefab;
WActionDescriptorHandle WSelectionActions::s_hConvertToEditorPrefab;
WActionDescriptorHandle WSelectionActions::s_hCopyReference;
WActionDescriptorHandle WSelectionActions::s_hSelectParent;
WActionDescriptorHandle WSelectionActions::s_hSetActiveParent;
WActionDescriptorHandle WSelectionActions::s_hClearActiveParent;
WActionDescriptorHandle WSelectionActions::s_hUndoSelection;


void WSelectionActions::RegisterActions()
{
  s_hGroupSelectedItems = W_REGISTER_ACTION_1("Selection.GroupItems", WActionScope::Document, "Scene - Selection", "Ctrl+G", WSelectionAction,
    WSelectionAction::ActionType::GroupSelectedItems);
  s_hCreateEmptyChildObject = W_REGISTER_ACTION_1("Selection.CreateEmptyChildObject", WActionScope::Document, "Scene - Selection", "",
    WSelectionAction, WSelectionAction::ActionType::CreateEmptyChildObject);
  s_hSelectParent = W_REGISTER_ACTION_1("Selection.SelectParent", WActionScope::Document, "Scene - Selection", "Ctrl+Q",
    WSelectionAction, WSelectionAction::ActionType::SelectParent);
  s_hCreateEmptyObjectAtPosition = W_REGISTER_ACTION_1("Selection.CreateEmptyObjectAtPosition", WActionScope::Document, "Scene - Selection",
    "Ctrl+Shift+X", WSelectionAction, WSelectionAction::ActionType::CreateEmptyObjectAtPosition);
  s_hHideSelectedObjects = W_REGISTER_ACTION_1(
    "Selection.HideItems", WActionScope::Document, "Scene - Selection", "H", WSelectionAction, WSelectionAction::ActionType::HideSelectedObjects);
  s_hHideUnselectedObjects = W_REGISTER_ACTION_1("Selection.HideUnselectedItems", WActionScope::Document, "Scene - Selection", "Shift+H",
    WSelectionAction, WSelectionAction::ActionType::HideUnselectedObjects);
  s_hShowHiddenObjects = W_REGISTER_ACTION_1("Selection.ShowHidden", WActionScope::Document, "Scene - Selection", "Ctrl+H", WSelectionAction,
    WSelectionAction::ActionType::ShowHiddenObjects);
  s_hAttachToObject = W_REGISTER_ACTION_1(
    "Selection.Attach", WActionScope::Document, "Scene - Selection", "", WSelectionAction, WSelectionAction::ActionType::AttachToObject);
  s_hDetachFromParent = W_REGISTER_ACTION_1(
    "Selection.Detach", WActionScope::Document, "Scene - Selection", "", WSelectionAction, WSelectionAction::ActionType::DetachFromParent);

  s_hPrefabMenu = W_REGISTER_MENU_WITH_ICON("Prefabs.Menu", ":/AssetIcons/Prefab.svg");
  s_hCreatePrefab =
    W_REGISTER_ACTION_1("Prefabs.Create", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::CreatePrefab);
  s_hRevertPrefab =
    W_REGISTER_ACTION_1("Prefabs.Revert", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::RevertPrefab);
  s_hUnlinkFromPrefab = W_REGISTER_ACTION_1(
    "Prefabs.Unlink", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::UnlinkFromPrefab);
  s_hOpenPrefabDocument = W_REGISTER_ACTION_1(
    "Prefabs.OpenDocument", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::OpenPrefabDocument);
  s_hConvertToEnginePrefab = W_REGISTER_ACTION_1(
    "Prefabs.ConvertToEngine", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::ConvertToEnginePrefab);
  s_hConvertToEditorPrefab = W_REGISTER_ACTION_1(
    "Prefabs.ConvertToEditor", WActionScope::Document, "Prefabs", "", WSelectionAction, WSelectionAction::ActionType::ConvertToEditorPrefab);

  s_hDuplicateSpecial = W_REGISTER_ACTION_1("Selection.DuplicateSpecial", WActionScope::Document, "Scene - Selection", "Ctrl+D", WSelectionAction,
    WSelectionAction::ActionType::DuplicateSpecial);
  s_hDeltaTransform = W_REGISTER_ACTION_1("Selection.DeltaTransform", WActionScope::Document, "Scene - Selection", "Ctrl+M", WSelectionAction,
    WSelectionAction::ActionType::DeltaTransform);
  s_hSnapObjectToCamera = W_REGISTER_ACTION_1(
    "Scene.Camera.SnapObjectToCamera", WActionScope::Document, "Camera", "", WSelectionAction, WSelectionAction::ActionType::SnapObjectToCamera);
  s_hCopyReference = W_REGISTER_ACTION_1(
    "Selection.CopyReference", WActionScope::Document, "Scene - Selection", "", WSelectionAction, WSelectionAction::ActionType::CopyReference);

  s_hSetActiveParent = W_REGISTER_ACTION_1("Selection.SetActiveParent", WActionScope::Document, "Scene - Selection", "Ctrl+Shift+A", WSelectionAction, WSelectionAction::ActionType::SetActiveParent);
  s_hClearActiveParent = W_REGISTER_ACTION_1("Selection.ClearActiveParent", WActionScope::Document, "Scene - Selection", "Ctrl+Shift+C", WSelectionAction, WSelectionAction::ActionType::ClearActiveParent);

  s_hUndoSelection = W_REGISTER_ACTION_1("Selection.UndoSelection", WActionScope::Document, "Scene - Selection", "Ctrl+B", WSelectionAction, WSelectionAction::ActionType::UndoSelection);
}

void WSelectionActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hGroupSelectedItems);
  WActionManager::UnregisterAction(s_hCreateEmptyChildObject);
  WActionManager::UnregisterAction(s_hCreateEmptyObjectAtPosition);
  WActionManager::UnregisterAction(s_hHideSelectedObjects);
  WActionManager::UnregisterAction(s_hHideUnselectedObjects);
  WActionManager::UnregisterAction(s_hShowHiddenObjects);
  WActionManager::UnregisterAction(s_hPrefabMenu);
  WActionManager::UnregisterAction(s_hCreatePrefab);
  WActionManager::UnregisterAction(s_hRevertPrefab);
  WActionManager::UnregisterAction(s_hUnlinkFromPrefab);
  WActionManager::UnregisterAction(s_hOpenPrefabDocument);
  WActionManager::UnregisterAction(s_hDuplicateSpecial);
  WActionManager::UnregisterAction(s_hDeltaTransform);
  WActionManager::UnregisterAction(s_hSnapObjectToCamera);
  WActionManager::UnregisterAction(s_hAttachToObject);
  WActionManager::UnregisterAction(s_hDetachFromParent);
  WActionManager::UnregisterAction(s_hConvertToEditorPrefab);
  WActionManager::UnregisterAction(s_hConvertToEnginePrefab);
  WActionManager::UnregisterAction(s_hCopyReference);
  WActionManager::UnregisterAction(s_hSelectParent);
  WActionManager::UnregisterAction(s_hSetActiveParent);
  WActionManager::UnregisterAction(s_hClearActiveParent);
  WActionManager::UnregisterAction(s_hUndoSelection);
}

void WSelectionActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 1.0f);
  pMap->MapAction(s_hCreateEmptyObjectAtPosition, "G.Selection", 1.1f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 3.7f);
  pMap->MapAction(s_hSelectParent, "G.Selection", 3.8f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 4.0f);
  pMap->MapAction(s_hHideUnselectedObjects, "G.Selection", 5.0f);
  pMap->MapAction(s_hShowHiddenObjects, "G.Selection", 6.0f);
  pMap->MapAction(s_hDuplicateSpecial, "G.Selection", 7.0f);
  pMap->MapAction(s_hDeltaTransform, "G.Selection", 7.1f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 7.2f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 7.3f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 9.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 10.0f);
  pMap->MapAction(s_hSetActiveParent, "G.Selection", 11.0f);
  pMap->MapAction(s_hClearActiveParent, "G.Selection", 12.0f);
  pMap->MapAction(s_hUndoSelection, "CmdHistoryCategory", 13.0f);

  MapPrefabActions(sMapping, 0.0f);
}

void WSelectionActions::MapPrefabActions(WStringView sMapping, float fPriority)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hPrefabMenu, "G.Selection", fPriority);

  pMap->MapAction(s_hOpenPrefabDocument, "G.Selection", "Prefabs.Menu", 1.0f);
  pMap->MapAction(s_hRevertPrefab, "G.Selection", "Prefabs.Menu", 2.0f);
  pMap->MapAction(s_hCreatePrefab, "G.Selection", "Prefabs.Menu", 3.0f);
  pMap->MapAction(s_hUnlinkFromPrefab, "G.Selection", "Prefabs.Menu", 4.0f);
  pMap->MapAction(s_hConvertToEditorPrefab, "G.Selection", "Prefabs.Menu", 5.0f);
  pMap->MapAction(s_hConvertToEnginePrefab, "G.Selection", "Prefabs.Menu", 6.0f);
}

void WSelectionActions::MapContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyChildObject, "G.Selection", 0.5f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hSelectParent, "G.Selection", 2.5f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 3.0f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 3.2f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 4.0f);
  pMap->MapAction(s_hSetActiveParent, "G.Selection", 11.0f);
  pMap->MapAction(s_hClearActiveParent, "G.Selection", 12.0f);

  MapPrefabActions(sMapping, 4.0f);
}

void WSelectionActions::MapViewContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCreateEmptyObjectAtPosition, "G.Selection", 1.0f);
  pMap->MapAction(s_hGroupSelectedItems, "G.Selection", 2.0f);
  pMap->MapAction(s_hSelectParent, "G.Selection", 3.0f);
  pMap->MapAction(s_hHideSelectedObjects, "G.Selection", 4.0f);
  pMap->MapAction(s_hAttachToObject, "G.Selection", 5.0f);
  pMap->MapAction(s_hDetachFromParent, "G.Selection", 6.0f);
  pMap->MapAction(s_hSnapObjectToCamera, "G.Selection", 7.0f);
  pMap->MapAction(s_hCopyReference, "G.Selection", 10.0f);


  MapPrefabActions(sMapping, 12.0f);
}

WSelectionAction::WSelectionAction(const WActionContext& context, const char* szName, WSelectionAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<WSceneDocument*>(static_cast<const WSceneDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      SetIconPath(":/EditorPluginScene/Icons/GroupSelection.svg");
      break;
    case ActionType::CreateEmptyChildObject:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::CreateEmptyObjectAtPosition:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode.svg");
      break;
    case ActionType::HideSelectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideSelected.svg");
      break;
    case ActionType::HideUnselectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideUnselected.svg");
      break;
    case ActionType::ShowHiddenObjects:
      SetIconPath(":/EditorPluginScene/Icons/ShowHidden.svg");
      break;
    case ActionType::CreatePrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabCreate.svg");
      break;
    case ActionType::RevertPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabRevert.svg");
      break;
    case ActionType::UnlinkFromPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUnlink.svg");
      break;
    case ActionType::OpenPrefabDocument:
      SetIconPath(":/EditorPluginScene/Icons/PrefabOpenDocument.svg");
      break;
    case ActionType::DuplicateSpecial:
      SetIconPath(":/EditorPluginScene/Icons/Duplicate.svg");
      break;
    case ActionType::DeltaTransform:
      // SetIconPath(":/EditorPluginScene/Icons/DeltaTransform.svg"); // TODO Icon
      break;
    case ActionType::SnapObjectToCamera:
      // SetIconPath(":/EditorPluginScene/Icons/SnapToCamera.svg"); // TODO Icon
      break;
    case ActionType::AttachToObject:
      // SetIconPath(":/EditorPluginScene/Icons/Attach.svg"); // TODO Icon
      break;
    case ActionType::DetachFromParent:
      // SetIconPath(":/EditorPluginScene/Icons/Detach.svg"); // TODO Icon
      break;
    case ActionType::ConvertToEditorPrefab:
      // SetIconPath(":/EditorPluginScene/ToEditorPrefab.png"); // TODO Icon
      break;
    case ActionType::ConvertToEnginePrefab:
      // SetIconPath(":/EditorPluginScene/ToEnginePrefab.png"); // TODO Icon
      break;
    case ActionType::CopyReference:
      SetIconPath(":/EditorFramework/Icons/id.svg");
      break;
    case ActionType::SelectParent:
      SetIconPath(":/EditorPluginScene/Icons/SelectParent.svg");
      break;
    case ActionType::SetActiveParent:
      // SetIconPath(":/EditorPluginScene/Icons/SelectParent.svg"); // TODO Icon
      break;
    case ActionType::ClearActiveParent:
      // SetIconPath(":/EditorPluginScene/Icons/SelectParent.svg"); // TODO Icon
      break;
    case ActionType::UndoSelection:
      // SetIconPath(":/EditorPluginScene/Icons/SelectParent.svg"); // TODO Icon
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WSelectionAction::SelectionEventHandler, this));
}


WSelectionAction::~WSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WSelectionAction::SelectionEventHandler, this));
}

void WSelectionAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      m_pSceneDocument->GroupSelection();
      return;
    case ActionType::CreateEmptyChildObject:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(true, false, false);
      WQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::CreateEmptyObjectAtPosition:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(false, true, true);
      WQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::HideSelectedObjects:
      m_pSceneDocument->ShowOrHideSelectedObjects(WSceneDocument::ShowOrHide::Hide);
      m_pSceneDocument->ShowDocumentStatus("Hiding selected objects");
      break;
    case ActionType::HideUnselectedObjects:
      m_pSceneDocument->HideUnselectedObjects();
      m_pSceneDocument->ShowDocumentStatus("Hiding unselected objects");
      break;
    case ActionType::ShowHiddenObjects:
      m_pSceneDocument->ShowOrHideAllObjects(WSceneDocument::ShowOrHide::Show);
      m_pSceneDocument->ShowDocumentStatus("Showing hidden objects");
      break;
    case ActionType::CreatePrefab:
      CreatePrefab();
      break;

    case ActionType::RevertPrefab:
    {
      if (WQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and revert to the prefab template state?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        WTempHybridArray<WSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(WGetStaticRTTI<WGameObject>(), selection);

        WTempHybridArray<const WDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (WUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->RevertPrefabs(selection2);
      }
    }
    break;

    case ActionType::UnlinkFromPrefab:
    {
      if (WQtUiServices::MessageBoxQuestion("Unlink the selected prefab instances from their templates?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        WTempHybridArray<WSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(WGetStaticRTTI<WGameObject>(), selection);

        WTempHybridArray<const WDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (WUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->UnlinkPrefabs(selection2);
      }
    }
    break;

    case ActionType::OpenPrefabDocument:
      OpenPrefabDocument();
      break;

    case ActionType::DuplicateSpecial:
      m_pSceneDocument->DuplicateSpecial();
      break;

    case ActionType::DeltaTransform:
      m_pSceneDocument->DeltaTransform();
      break;

    case ActionType::SnapObjectToCamera:
      m_pSceneDocument->SnapObjectToCamera();
      break;

    case ActionType::AttachToObject:
      m_pSceneDocument->AttachToObject();
      break;
    case ActionType::DetachFromParent:
      m_pSceneDocument->DetachFromParent();
      break;

    case ActionType::CopyReference:
      m_pSceneDocument->CopyReference();
      break;

    case ActionType::ConvertToEditorPrefab:
    {
      WTempHybridArray<WSelectionEntry, 64> selection;
      m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(WGetStaticRTTI<WGameObject>(), selection);

      WTempHybridArray<const WDocumentObject*, 64> selection2;
      selection2.SetCount(selection.GetCount());
      for (WUInt32 i = 0; i < selection.GetCount(); ++i)
      {
        selection2[i] = selection[i].m_pObject;
      }

      m_pSceneDocument->ConvertToEditorPrefab(selection2);
    }
    break;

    case ActionType::ConvertToEnginePrefab:
    {
      if (WQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and convert them to engine prefabs?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        WTempHybridArray<WSelectionEntry, 64> selection;
        m_pSceneDocument->GetSelectionManager()->GetTopLevelSelectionOfType(WGetStaticRTTI<WGameObject>(), selection);

        WTempHybridArray<const WDocumentObject*, 64> selection2;
        selection2.SetCount(selection.GetCount());
        for (WUInt32 i = 0; i < selection.GetCount(); ++i)
        {
          selection2[i] = selection[i].m_pObject;
        }

        m_pSceneDocument->ConvertToEnginePrefab(selection2);
      }
    }
    break;

    case ActionType::SelectParent:
    {
      m_pSceneDocument->SelectParentObject();
      return;
    }

    case ActionType::SetActiveParent:
    {
      m_pSceneDocument->SetSelectedAsActiveParent();
      return;
    }

    case ActionType::ClearActiveParent:
    {
      m_pSceneDocument->ClearActiveParent();
      return;
    }

    case ActionType::UndoSelection:
    {
      m_pSceneDocument->UndoSelection();
      return;
    }
  }
}


void WSelectionAction::OpenPrefabDocument()
{
  const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

  if (sel.GetCount() != 1)
    return;

  const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);


  WUuid PrefabAsset;
  if (pScene->IsObjectEnginePrefab(sel[0]->GetGuid(), &PrefabAsset))
  {
    // PrefabAsset is all we need
  }
  else
  {
    auto pMeta = pScene->m_DocumentObjectMetaData->BeginReadMetaData(sel[0]->GetGuid());
    PrefabAsset = pMeta->m_CreateFromPrefab;
    pScene->m_DocumentObjectMetaData->EndReadMetaData();
  }

  auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(PrefabAsset);
  if (pAsset)
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(pAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
  }
  else
  {
    WQtUiServices::MessageBoxWarning("The prefab asset of this instance is currently unknown. It may have been deleted. Try updating the "
                                      "asset library ('Check FileSystem'), if it should be there.");
  }
}

void WSelectionAction::CreatePrefab()
{
  static WString sSearchDir = WToolsProject::GetSingleton()->GetProjectFile();

  WStringBuilder sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Prefab"),
    QString::fromUtf8(sSearchDir.GetData()), QString::fromUtf8("*.WPrefab"), nullptr, QFileDialog::Option::DontResolveSymlinks)
                            .toUtf8()
                            .data();

  if (!sFile.IsEmpty())
  {
    sFile.ChangeFileExtension("WPrefab");

    sSearchDir = sFile.GetFileDirectory();

    if (WOSFile::ExistsFile(sFile))
    {
      WQtUiServices::MessageBoxInformation("You currently cannot replace an existing prefab this way. Please choose a new prefab file.");
      return;
    }

    auto res = m_pSceneDocument->CreatePrefabDocumentFromSelection(sFile, WGetStaticRTTI<WGameObject>());
    m_pSceneDocument->ScheduleSendObjectSelection(); // fix selection of prefab object
    WQtUiServices::MessageBoxStatus(res, "Failed to create Prefab", "Successfully created Prefab");
  }
}

void WSelectionAction::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void WSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::HideSelectedObjects || m_Type == ActionType::DuplicateSpecial || m_Type == ActionType::DeltaTransform ||
      m_Type == ActionType::SnapObjectToCamera || m_Type == ActionType::DetachFromParent || m_Type == ActionType::HideUnselectedObjects ||
      m_Type == ActionType::AttachToObject)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
  else if (m_Type == ActionType::GroupSelectedItems)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() > 1);
  }
  else if (m_Type == ActionType::CreateEmptyChildObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() <= 1);
  }
  else if (m_Type == ActionType::CopyReference)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
  else if (m_Type == ActionType::SelectParent)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
  else if (m_Type == ActionType::SetActiveParent)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() >= 1);
  }
  else if (m_Type == ActionType::ClearActiveParent)
  {
    const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);
    SetEnabled(pScene->GetActiveParent().IsValid());
  }
  else if (m_Type == ActionType::UndoSelection)
  {
    const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);
    SetEnabled(pScene->CanUndoSelection());
  }
  else if (m_Type == ActionType::OpenPrefabDocument)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.GetCount() != 1)
    {
      SetEnabled(false);
      return;
    }

    const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid()) || pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
    return;
  }
  else if (m_Type == ActionType::RevertPrefab || m_Type == ActionType::UnlinkFromPrefab || m_Type == ActionType::ConvertToEnginePrefab ||
           m_Type == ActionType::CreatePrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    if (m_Type == ActionType::CreatePrefab)
    {
      SetEnabled(true);
      return;
    }

    const bool bShouldBePrefab =
      (m_Type == ActionType::RevertPrefab) || (m_Type == ActionType::ConvertToEnginePrefab) || (m_Type == ActionType::UnlinkFromPrefab);

    const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab == bShouldBePrefab);
  }
  else if (m_Type == ActionType::ConvertToEditorPrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    const WSceneDocument* pScene = static_cast<const WSceneDocument*>(m_Context.m_pDocument);
    const bool bIsPrefab = pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
  }
}
