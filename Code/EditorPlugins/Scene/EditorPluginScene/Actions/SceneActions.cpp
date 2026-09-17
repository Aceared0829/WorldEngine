#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Dialogs/ExportAndRunDlg.moc.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QProcess>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WSceneActions::s_hSceneCategory;
WActionDescriptorHandle WSceneActions::s_hSceneUtilsMenu;
WActionDescriptorHandle WSceneActions::s_hExportScene;
WActionDescriptorHandle WSceneActions::s_hGameModeSimulate;
WActionDescriptorHandle WSceneActions::s_hGameModePlay;
WActionDescriptorHandle WSceneActions::s_hGameModePlayFromHere;
WActionDescriptorHandle WSceneActions::s_hGameModeStop;
WActionDescriptorHandle WSceneActions::s_hGameModePause;
WActionDescriptorHandle WSceneActions::s_hUtilExportSceneToOBJ;
WActionDescriptorHandle WSceneActions::s_hKeepSimulationChanges;
WActionDescriptorHandle WSceneActions::s_hCreateThumbnail;
WActionDescriptorHandle WSceneActions::s_hFavoriteCamsMenu;
WActionDescriptorHandle WSceneActions::s_hStoreEditorCamera[10];
WActionDescriptorHandle WSceneActions::s_hRestoreEditorCamera[10];
WActionDescriptorHandle WSceneActions::s_hJumpToCamera[10];
WActionDescriptorHandle WSceneActions::s_hCreateLevelCamera[10];

void WSceneActions::RegisterActions()
{
  s_hSceneCategory = W_REGISTER_CATEGORY("SceneCategory");
  s_hSceneUtilsMenu = W_REGISTER_MENU_WITH_ICON("Scene.Utils.Menu", "");

  s_hExportScene = W_REGISTER_ACTION_1("Scene.ExportAndRun", WActionScope::Document, "Scene", "Ctrl+R", WSceneAction, WSceneAction::ActionType::ExportAndRunScene);
  s_hGameModeSimulate = W_REGISTER_ACTION_1("Scene.GameMode.Simulate", WActionScope::Document, "Scene", "F5", WSceneAction, WSceneAction::ActionType::StartGameModeSimulate);
  s_hGameModePlay = W_REGISTER_ACTION_1("Scene.GameMode.Play", WActionScope::Document, "Scene", "Ctrl+F5", WSceneAction, WSceneAction::ActionType::StartGameModePlay);

  s_hGameModePlayFromHere = W_REGISTER_ACTION_1("Scene.GameMode.PlayFromHere", WActionScope::Document, "Scene", "F6", WSceneAction,
    WSceneAction::ActionType::StartGameModePlayFromHere);

  s_hGameModeStop = W_REGISTER_ACTION_1("Scene.GameMode.Stop", WActionScope::Document, "Scene", "Shift+F5", WSceneAction, WSceneAction::ActionType::StopGameMode);

  s_hGameModePause = W_REGISTER_ACTION_1("Scene.GameMode.Pause", WActionScope::Document, "Scene", "Pause", WSceneAction, WSceneAction::ActionType::PauseSimulation);

  s_hUtilExportSceneToOBJ = W_REGISTER_ACTION_1("Scene.ExportSceneToOBJ", WActionScope::Document, "Scene", "", WSceneAction, WSceneAction::ActionType::ExportSceneToOBJ);

  s_hKeepSimulationChanges = W_REGISTER_ACTION_1("Scene.KeepSimulationChanges", WActionScope::Document, "Scene", "K", WSceneAction, WSceneAction::ActionType::KeepSimulationChanges);

  s_hCreateThumbnail = W_REGISTER_ACTION_1("Scene.CreateThumbnail", WActionScope::Document, "Scene", "", WSceneAction, WSceneAction::ActionType::CreateThumbnail);
  // unfortunately the macros use lambdas thus using a loop to generate the strings does not work
  {
    s_hFavoriteCamsMenu = W_REGISTER_MENU_WITH_ICON("Scene.FavoriteCams.Menu", "");

    s_hStoreEditorCamera[0] = W_REGISTER_ACTION_1("Scene.Camera.Store.0", WActionScope::Document, "Scene - Cameras", "Ctrl+0", WSceneAction, WSceneAction::ActionType::StoreEditorCamera0);
    s_hStoreEditorCamera[1] = W_REGISTER_ACTION_1("Scene.Camera.Store.1", WActionScope::Document, "Scene - Cameras", "Ctrl+1", WSceneAction, WSceneAction::ActionType::StoreEditorCamera1);
    s_hStoreEditorCamera[2] = W_REGISTER_ACTION_1("Scene.Camera.Store.2", WActionScope::Document, "Scene - Cameras", "Ctrl+2", WSceneAction, WSceneAction::ActionType::StoreEditorCamera2);
    s_hStoreEditorCamera[3] = W_REGISTER_ACTION_1("Scene.Camera.Store.3", WActionScope::Document, "Scene - Cameras", "Ctrl+3", WSceneAction, WSceneAction::ActionType::StoreEditorCamera3);
    s_hStoreEditorCamera[4] = W_REGISTER_ACTION_1("Scene.Camera.Store.4", WActionScope::Document, "Scene - Cameras", "Ctrl+4", WSceneAction, WSceneAction::ActionType::StoreEditorCamera4);
    s_hStoreEditorCamera[5] = W_REGISTER_ACTION_1("Scene.Camera.Store.5", WActionScope::Document, "Scene - Cameras", "Ctrl+5", WSceneAction, WSceneAction::ActionType::StoreEditorCamera5);
    s_hStoreEditorCamera[6] = W_REGISTER_ACTION_1("Scene.Camera.Store.6", WActionScope::Document, "Scene - Cameras", "Ctrl+6", WSceneAction, WSceneAction::ActionType::StoreEditorCamera6);
    s_hStoreEditorCamera[7] = W_REGISTER_ACTION_1("Scene.Camera.Store.7", WActionScope::Document, "Scene - Cameras", "Ctrl+7", WSceneAction, WSceneAction::ActionType::StoreEditorCamera7);
    s_hStoreEditorCamera[8] = W_REGISTER_ACTION_1("Scene.Camera.Store.8", WActionScope::Document, "Scene - Cameras", "Ctrl+8", WSceneAction, WSceneAction::ActionType::StoreEditorCamera8);
    s_hStoreEditorCamera[9] = W_REGISTER_ACTION_1("Scene.Camera.Store.9", WActionScope::Document, "Scene - Cameras", "Ctrl+9", WSceneAction, WSceneAction::ActionType::StoreEditorCamera9);

    s_hRestoreEditorCamera[0] = W_REGISTER_ACTION_1("Scene.Camera.Restore.0", WActionScope::Document, "Scene - Cameras", "0", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera0);
    s_hRestoreEditorCamera[1] = W_REGISTER_ACTION_1("Scene.Camera.Restore.1", WActionScope::Document, "Scene - Cameras", "1", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera1);
    s_hRestoreEditorCamera[2] = W_REGISTER_ACTION_1("Scene.Camera.Restore.2", WActionScope::Document, "Scene - Cameras", "2", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera2);
    s_hRestoreEditorCamera[3] = W_REGISTER_ACTION_1("Scene.Camera.Restore.3", WActionScope::Document, "Scene - Cameras", "3", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera3);
    s_hRestoreEditorCamera[4] = W_REGISTER_ACTION_1("Scene.Camera.Restore.4", WActionScope::Document, "Scene - Cameras", "4", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera4);
    s_hRestoreEditorCamera[5] = W_REGISTER_ACTION_1("Scene.Camera.Restore.5", WActionScope::Document, "Scene - Cameras", "5", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera5);
    s_hRestoreEditorCamera[6] = W_REGISTER_ACTION_1("Scene.Camera.Restore.6", WActionScope::Document, "Scene - Cameras", "6", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera6);
    s_hRestoreEditorCamera[7] = W_REGISTER_ACTION_1("Scene.Camera.Restore.7", WActionScope::Document, "Scene - Cameras", "7", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera7);
    s_hRestoreEditorCamera[8] = W_REGISTER_ACTION_1("Scene.Camera.Restore.8", WActionScope::Document, "Scene - Cameras", "8", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera8);
    s_hRestoreEditorCamera[9] = W_REGISTER_ACTION_1("Scene.Camera.Restore.9", WActionScope::Document, "Scene - Cameras", "9", WSceneAction, WSceneAction::ActionType::RestoreEditorCamera9);

    s_hJumpToCamera[0] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.0", WActionScope::Document, "Scene - Cameras", "Alt+0", WSceneAction, WSceneAction::ActionType::JumpToCamera0);
    s_hJumpToCamera[1] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.1", WActionScope::Document, "Scene - Cameras", "Alt+1", WSceneAction, WSceneAction::ActionType::JumpToCamera1);
    s_hJumpToCamera[2] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.2", WActionScope::Document, "Scene - Cameras", "Alt+2", WSceneAction, WSceneAction::ActionType::JumpToCamera2);
    s_hJumpToCamera[3] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.3", WActionScope::Document, "Scene - Cameras", "Alt+3", WSceneAction, WSceneAction::ActionType::JumpToCamera3);
    s_hJumpToCamera[4] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.4", WActionScope::Document, "Scene - Cameras", "Alt+4", WSceneAction, WSceneAction::ActionType::JumpToCamera4);
    s_hJumpToCamera[5] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.5", WActionScope::Document, "Scene - Cameras", "Alt+5", WSceneAction, WSceneAction::ActionType::JumpToCamera5);
    s_hJumpToCamera[6] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.6", WActionScope::Document, "Scene - Cameras", "Alt+6", WSceneAction, WSceneAction::ActionType::JumpToCamera6);
    s_hJumpToCamera[7] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.7", WActionScope::Document, "Scene - Cameras", "Alt+7", WSceneAction, WSceneAction::ActionType::JumpToCamera7);
    s_hJumpToCamera[8] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.8", WActionScope::Document, "Scene - Cameras", "Alt+8", WSceneAction, WSceneAction::ActionType::JumpToCamera8);
    s_hJumpToCamera[9] = W_REGISTER_ACTION_1("Scene.Camera.JumpTo.9", WActionScope::Document, "Scene - Cameras", "Alt+9", WSceneAction, WSceneAction::ActionType::JumpToCamera9);

    s_hCreateLevelCamera[0] = W_REGISTER_ACTION_1("Scene.Camera.Create.0", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+0", WSceneAction, WSceneAction::ActionType::CreateLevelCamera0);
    s_hCreateLevelCamera[1] = W_REGISTER_ACTION_1("Scene.Camera.Create.1", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+1", WSceneAction, WSceneAction::ActionType::CreateLevelCamera1);
    s_hCreateLevelCamera[2] = W_REGISTER_ACTION_1("Scene.Camera.Create.2", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+2", WSceneAction, WSceneAction::ActionType::CreateLevelCamera2);
    s_hCreateLevelCamera[3] = W_REGISTER_ACTION_1("Scene.Camera.Create.3", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+3", WSceneAction, WSceneAction::ActionType::CreateLevelCamera3);
    s_hCreateLevelCamera[4] = W_REGISTER_ACTION_1("Scene.Camera.Create.4", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+4", WSceneAction, WSceneAction::ActionType::CreateLevelCamera4);
    s_hCreateLevelCamera[5] = W_REGISTER_ACTION_1("Scene.Camera.Create.5", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+5", WSceneAction, WSceneAction::ActionType::CreateLevelCamera5);
    s_hCreateLevelCamera[6] = W_REGISTER_ACTION_1("Scene.Camera.Create.6", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+6", WSceneAction, WSceneAction::ActionType::CreateLevelCamera6);
    s_hCreateLevelCamera[7] = W_REGISTER_ACTION_1("Scene.Camera.Create.7", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+7", WSceneAction, WSceneAction::ActionType::CreateLevelCamera7);
    s_hCreateLevelCamera[8] = W_REGISTER_ACTION_1("Scene.Camera.Create.8", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+8", WSceneAction, WSceneAction::ActionType::CreateLevelCamera8);
    s_hCreateLevelCamera[9] = W_REGISTER_ACTION_1("Scene.Camera.Create.9", WActionScope::Document, "Scene - Cameras", "Ctrl+Alt+9", WSceneAction, WSceneAction::ActionType::CreateLevelCamera9);
  }
}

void WSceneActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hSceneCategory);
  WActionManager::UnregisterAction(s_hSceneUtilsMenu);
  WActionManager::UnregisterAction(s_hExportScene);
  WActionManager::UnregisterAction(s_hGameModeSimulate);
  WActionManager::UnregisterAction(s_hGameModePlay);
  WActionManager::UnregisterAction(s_hGameModePlayFromHere);
  WActionManager::UnregisterAction(s_hGameModeStop);
  WActionManager::UnregisterAction(s_hGameModePause);
  WActionManager::UnregisterAction(s_hUtilExportSceneToOBJ);
  WActionManager::UnregisterAction(s_hKeepSimulationChanges);
  WActionManager::UnregisterAction(s_hCreateThumbnail);
  WActionManager::UnregisterAction(s_hFavoriteCamsMenu);

  for (WUInt32 i = 0; i < 10; ++i)
  {
    WActionManager::UnregisterAction(s_hStoreEditorCamera[i]);
    WActionManager::UnregisterAction(s_hRestoreEditorCamera[i]);
    WActionManager::UnregisterAction(s_hJumpToCamera[i]);
    WActionManager::UnregisterAction(s_hCreateLevelCamera[i]);
  }
}

void WSceneActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "G.Scene/SceneCategory";
    const char* szUtilsSubPath = "G.Scene/Scene.Utils.Menu";

    pMap->MapAction(s_hSceneUtilsMenu, "G.Scene", 2.0f);
    // pMap->MapAction(s_hCreateThumbnail, szUtilsSubPath, 0.0f); // now available through the export scene dialog
    pMap->MapAction(s_hKeepSimulationChanges, szUtilsSubPath, 1.0f);
    pMap->MapAction(s_hUtilExportSceneToOBJ, szUtilsSubPath, 2.0f);

    pMap->MapAction(s_hFavoriteCamsMenu, "G.Scene", 3.0f);
    const char* szFavCamsSubPath = "G.Scene/Scene.FavoriteCams.Menu";

    for (WUInt32 i = 0; i < 10; ++i)
    {
      pMap->MapAction(s_hStoreEditorCamera[i], szFavCamsSubPath, 10.0f + i);
      pMap->MapAction(s_hRestoreEditorCamera[i], szFavCamsSubPath, 20.0f + i);
      pMap->MapAction(s_hJumpToCamera[i], szFavCamsSubPath, 30.0f + i);
      pMap->MapAction(s_hCreateLevelCamera[i], szFavCamsSubPath, 40.0f + i);
    }

    pMap->MapAction(s_hSceneCategory, "G.Scene", 4.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);
    pMap->MapAction(s_hGameModePlayFromHere, szSubPath, 7.0f);
    pMap->MapAction(s_hGameModePause, szSubPath, 8.0f);
  }
}

void WSceneActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";


    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 11.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModePause, szSubPath, 1.5f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 3.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 4.0f);
  }
}

void WSceneActions::MapViewContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGameModePlayFromHere, "", 0.0f);
}

WSceneAction::WSceneAction(const WActionContext& context, const char* szName, WSceneAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSceneDocument = static_cast<WSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
      SetIconPath(":/EditorPluginScene/Icons/SceneExport.svg");
      break;

    case ActionType::StartGameModeSimulate:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlay.svg");
      SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Play);
      break;

    case ActionType::StartGameModePlay:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg");
      break;

    case ActionType::StartGameModePlayFromHere:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame.svg"); // TODO: icon
      break;

    case ActionType::StopGameMode:
      SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg");
      break;

    case ActionType::PauseSimulation:
      SetIconPath(":/EditorPluginScene/Icons/ScenePause.svg");
      break;

    case ActionType::ExportSceneToOBJ:
      // SetIconPath(":/EditorPluginScene/Icons/SceneStop.svg"); // TODO: icon
      break;

    case ActionType::KeepSimulationChanges:
      SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg");
      break;

    case ActionType::CreateThumbnail:
      // SetIconPath(":/EditorPluginScene/Icons/PullObjectState.svg"); // TODO: icon
      break;

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
      SetIconPath(":/TypeIcons/WCameraComponent.svg");
      break;

    default:
      // no icon
      break;
  }

  UpdateState();
}

WSceneAction::~WSceneAction()
{
  m_pSceneDocument->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WSceneAction::SceneEventHandler, this));
}

void WSceneAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ExportAndRunScene:
    {
      WStringBuilder sCmd;
      GetPlayerCommandLine(sCmd);

      WQtExportAndRunDlg dlg(nullptr);
      dlg.m_sCmdLine = sCmd;
      dlg.s_bUpdateThumbnail = false;
      dlg.m_bShowThumbnailCheckbox = !m_pSceneDocument->IsPrefab();

      if (dlg.exec() != QDialog::Accepted)
        return;

      WProgressRange range("Export and Run", 4, true);

      range.BeginNextStep("Build C++");
      if (dlg.s_bCompileCpp)
      {
        if (WCppProject::EnsureCppPluginReady().Failed())
          return;
      }

      bool bDidTransformAll = false;

      range.BeginNextStep("Transform Assets");
      if (dlg.s_bTransformAll)
      {
        if (WAssetCurator::GetSingleton()->TransformAllAssets().Succeeded())
        {
          // once all assets have been transformed, disable it for the next export
          dlg.s_bTransformAll = false;
          bDidTransformAll = true;
        }
      }

      bool bCreateThumbnail = dlg.s_bUpdateThumbnail;

      range.BeginNextStep("Create Thumbnail");
      if (!m_pSceneDocument->IsPrefab() && !bCreateThumbnail)
      {
        // if the thumbnail doesn't exist, or is very old, update it anyway

        WStringBuilder sThumbnailPath = m_pSceneDocument->GetAssetDocumentManager()->GenerateResourceThumbnailPath(m_pSceneDocument->GetDocumentPath());

        WFileStats stat;
        if (WOSFile::GetFileStats(sThumbnailPath, stat).Failed())
        {
          bCreateThumbnail = true;
        }
        else
        {
          auto tNow = WTimestamp::CurrentTimestamp();
          auto tComp = stat.m_LastModificationTime + WTime::MakeFromHours(24) * 7;

          if (tComp.GetInt64(WSIUnitOfTime::Second) < tNow.GetInt64(WSIUnitOfTime::Second))
          {
            bCreateThumbnail = true;
          }
        }
      }


      // Convert collections
      if (!bDidTransformAll)
      {
        WAssetCurator* pCurator = WAssetCurator::GetSingleton();
        pCurator->TransformAssetsForSceneExport(pCurator->GetActiveAssetProfile());
      }

      dlg.s_bUpdateThumbnail = false;

      range.BeginNextStep("Export Scene");
      if (m_pSceneDocument->ExportScene(bCreateThumbnail).Failed())
      {
        WQtUiServices::GetSingleton()->MessageBoxWarning("Scene export failed.");
        return;
      }

      // send event, so that 3rd party code can hook into this
      {
        WGameObjectDocumentEvent e;
        e.m_Type = WGameObjectDocumentEvent::Type::GameMode_StartingExternal;
        e.m_pDocument = m_pSceneDocument;
        m_pSceneDocument->s_GameObjectDocumentEvents.Broadcast(e);
      }

      if (dlg.m_bRunAfterExport)
      {
        LaunchPlayer(dlg.m_sApplication);
      }

      return;
    }

    case ActionType::StartGameModePlay:
      m_pSceneDocument->TriggerGameModePlay(false);
      return;

    case ActionType::StartGameModePlayFromHere:
      m_pSceneDocument->TriggerGameModePlay(true);
      return;

    case ActionType::StartGameModeSimulate:
      if (m_pSceneDocument->GetPauseSimulation())
      {
        m_pSceneDocument->SetPauseSimulation(false);
      }
      else
      {
        m_pSceneDocument->StartSimulateWorld();
      }
      return;

    case ActionType::StopGameMode:
      m_pSceneDocument->StopGameMode();
      return;

    case ActionType::PauseSimulation:
      if (m_pSceneDocument->GetPauseSimulation())
        m_pSceneDocument->StepSimulation();
      else
        m_pSceneDocument->PauseSimulation();
      return;

    case ActionType::ExportSceneToOBJ:
    {
      WQtExtractGeometryDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        m_pSceneDocument->ExportSceneGeometry(
          dlg.s_sDestinationFile.toUtf8().data(), dlg.s_bOnlySelection, dlg.s_iExtractionMode, dlg.GetCoordinateSystemTransform());
      }
      return;
    }

    case ActionType::KeepSimulationChanges:
    {
      WPullObjectStateMsgToEngine msg;
      m_pSceneDocument->SendMessageToEngine(&msg);
      return;
    }

    case ActionType::CreateThumbnail:
      m_pSceneDocument->ExportScene(true);
      return;

    case ActionType::StoreEditorCamera0:
    case ActionType::StoreEditorCamera1:
    case ActionType::StoreEditorCamera2:
    case ActionType::StoreEditorCamera3:
    case ActionType::StoreEditorCamera4:
    case ActionType::StoreEditorCamera5:
    case ActionType::StoreEditorCamera6:
    case ActionType::StoreEditorCamera7:
    case ActionType::StoreEditorCamera8:
    case ActionType::StoreEditorCamera9:
    {
      const WInt32 iCamIdx = (int)m_Type - (int)ActionType::StoreEditorCamera0;

      m_pSceneDocument->StoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(WFmt("Stored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::RestoreEditorCamera0:
    case ActionType::RestoreEditorCamera1:
    case ActionType::RestoreEditorCamera2:
    case ActionType::RestoreEditorCamera3:
    case ActionType::RestoreEditorCamera4:
    case ActionType::RestoreEditorCamera5:
    case ActionType::RestoreEditorCamera6:
    case ActionType::RestoreEditorCamera7:
    case ActionType::RestoreEditorCamera8:
    case ActionType::RestoreEditorCamera9:
    {
      const WInt32 iCamIdx = (int)m_Type - (int)ActionType::RestoreEditorCamera0;

      m_pSceneDocument->RestoreFavoriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(WFmt("Restored favorite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
    {
      const WInt32 iCamIdx = (int)m_Type - (int)ActionType::JumpToCamera0;

      const bool bImmediate = value.IsA<bool>() ? value.Get<bool>() : false;
      if (m_pSceneDocument->JumpToLevelCamera(iCamIdx, bImmediate).Failed())
      {
        m_pSceneDocument->ShowDocumentStatus(WFmt("No Camera Component found with shortcut set to '{0}'", iCamIdx));
      }
      return;
    }

    case ActionType::CreateLevelCamera0:
    case ActionType::CreateLevelCamera1:
    case ActionType::CreateLevelCamera2:
    case ActionType::CreateLevelCamera3:
    case ActionType::CreateLevelCamera4:
    case ActionType::CreateLevelCamera5:
    case ActionType::CreateLevelCamera6:
    case ActionType::CreateLevelCamera7:
    case ActionType::CreateLevelCamera8:
    case ActionType::CreateLevelCamera9:
    {
      const WInt32 iCamIdx = (int)m_Type - (int)ActionType::CreateLevelCamera0;

      if (auto pView = WQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;
          pView == nullptr || pView->m_pViewConfig->m_Perspective != WSceneViewPerspective::Perspective)
      {
        m_pSceneDocument->ShowDocumentStatus("Note: Level cameras cannot be created in orthographic views.");
        return;
      }

      if (m_pSceneDocument->CreateLevelCamera(iCamIdx).Succeeded())
      {
        m_pSceneDocument->ShowDocumentStatus(WFmt("Create level camera with shortcut set to '{0}'", iCamIdx));
      }
      else
      {
        m_pSceneDocument->ShowDocumentStatus(WFmt("Could not create level camera '{}'.", iCamIdx));
      }
      return;
    }
  }
}

void WSceneAction::LaunchPlayer(const char* szPlayerApp)
{
  WStringBuilder sCmd;
  QStringList arguments = GetPlayerCommandLine(sCmd);
  WLog::Info("Running: {} {}", szPlayerApp, sCmd);
  m_pSceneDocument->ShowDocumentStatus(WFmt("Running: {} {}", szPlayerApp, sCmd));

  WStringBuilder sPlayerApp = szPlayerApp;
#if W_ENABLED(W_PLATFORM_LINUX)
  if (sPlayerApp.IsRelativePath())
  {
    sPlayerApp.Prepend("./");
  }
#endif
  QProcess::startDetached(QString::fromUtf8(sPlayerApp.GetData()), arguments, QCoreApplication::applicationDirPath());
}

QStringList WSceneAction::GetPlayerCommandLine(WStringBuilder& out_sSingleLine) const
{
  QStringList arguments;
  arguments << "-project";
  arguments << WToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  {
    arguments << "-scene";

    WStringBuilder sAssetDataDir = WAssetCurator::GetSingleton()->FindDataDirectoryForAsset(m_pSceneDocument->GetDocumentPath());

    WStringBuilder sRelativePath = m_pSceneDocument->GetAssetDocumentManager()->GetAbsoluteOutputFileName(m_pSceneDocument->GetAssetDocumentTypeDescriptor(), m_pSceneDocument->GetDocumentPath(), "");

    sRelativePath.MakeRelativeTo(sAssetDataDir).AssertSuccess();
    sRelativePath.MakeCleanPath();

    arguments << sRelativePath.GetData();
  }

  WStringBuilder sWndCfgPath = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

  if (WOSFile::ExistsFile(sWndCfgPath))
  {
    arguments << "-wnd";
    arguments << QString::fromUtf8(sWndCfgPath, sWndCfgPath.GetElementCount());
  }

  arguments << "-profile";
  arguments << WString(WAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName()).GetData();

  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-renderer"))
  {
    WStringBuilder sRenderer = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    arguments << "-renderer";
    arguments << sRenderer.GetData();
  }

  for (QString s : arguments)
  {
    if (s.contains(" "))
      out_sSingleLine.AppendFormat(" \"{}\"", s.toUtf8().data());
    else
      out_sSingleLine.AppendFormat(" {}", s.toUtf8().data());
  }

  return arguments;
}

void WSceneAction::SceneEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::GameModeChanged:
    case WGameObjectEvent::Type::SimulationSpeedChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void WSceneAction::UpdateState()
{
  if (m_Type == ActionType::StartGameModeSimulate)
  {
    if (m_pSceneDocument->GetGameMode() == GameMode::Off)
      SetEnabled(true);
    else if (m_pSceneDocument->GetPauseSimulation() && (m_pSceneDocument->GetGameMode() == GameMode::Simulate || m_pSceneDocument->GetGameMode() == GameMode::Play))
      SetEnabled(true);
    else
      SetEnabled(false);
  }

  if (m_Type == ActionType::ExportAndRunScene || m_Type == ActionType::StartGameModePlay)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() == GameMode::Off);
  }

  if (m_Type == ActionType::StopGameMode)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);
  }

  if (m_Type == ActionType::PauseSimulation)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);

    if (m_pSceneDocument->GetPauseSimulation())
    {
      SetIconPath(":/EditorPluginScene/Icons/SceneStep.svg");
    }
    else
    {
      SetIconPath(":/EditorPluginScene/Icons/ScenePause.svg");
    }

    TriggerUpdate();
  }

  if (m_Type == ActionType::KeepSimulationChanges)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off && !m_pSceneDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
