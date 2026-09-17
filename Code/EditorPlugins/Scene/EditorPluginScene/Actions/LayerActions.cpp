#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QInputDialog>


// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WLayerActions::s_hLayerCategory;
WActionDescriptorHandle WLayerActions::s_hCreateLayer;
WActionDescriptorHandle WLayerActions::s_hDeleteLayer;
WActionDescriptorHandle WLayerActions::s_hSaveLayer;
WActionDescriptorHandle WLayerActions::s_hSaveActiveLayer;
WActionDescriptorHandle WLayerActions::s_hLayerLoaded;
WActionDescriptorHandle WLayerActions::s_hLayerVisible;
WActionDescriptorHandle WLayerActions::s_hSwitchOnSelection;

void WLayerActions::RegisterActions()
{
  s_hLayerCategory = W_REGISTER_CATEGORY("LayerCategory");

  s_hCreateLayer = W_REGISTER_ACTION_1("Layer.CreateLayer", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::CreateLayer);
  s_hDeleteLayer = W_REGISTER_ACTION_1("Layer.DeleteLayer", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::DeleteLayer);
  s_hSaveLayer = W_REGISTER_ACTION_1("Layer.SaveLayer", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::SaveLayer);
  s_hSaveActiveLayer = W_REGISTER_ACTION_1("Layer.SaveActiveLayer", WActionScope::Document, "Scene - Layer", "Ctrl+S",
    WLayerAction, WLayerAction::ActionType::SaveActiveLayer);
  s_hLayerLoaded = W_REGISTER_ACTION_1("Layer.LayerLoaded", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::LayerLoaded);
  s_hLayerVisible = W_REGISTER_ACTION_1("Layer.LayerVisible", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::LayerVisible);
  s_hSwitchOnSelection = W_REGISTER_ACTION_1("Layer.SwitchOnSelection", WActionScope::Document, "Scene - Layer", "",
    WLayerAction, WLayerAction::ActionType::SwitchOnSelection);
}

void WLayerActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hLayerCategory);
  WActionManager::UnregisterAction(s_hCreateLayer);
  WActionManager::UnregisterAction(s_hDeleteLayer);
  WActionManager::UnregisterAction(s_hSaveLayer);
  WActionManager::UnregisterAction(s_hSaveActiveLayer);
  WActionManager::UnregisterAction(s_hLayerLoaded);
  WActionManager::UnregisterAction(s_hLayerVisible);
  WActionManager::UnregisterAction(s_hSwitchOnSelection);
}

void WLayerActions::MapContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLayerCategory, "", 0.0f);

  const WStringView sSubPath = "LayerCategory";
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hSaveLayer, sSubPath, 3.0f);
  pMap->MapAction(s_hLayerLoaded, sSubPath, 4.0f);
  pMap->MapAction(s_hLayerVisible, sSubPath, 5.0f);
}

void WLayerActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLayerCategory, "", 0.0f);

  const WStringView sSubPath = "LayerCategory";
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hSwitchOnSelection, sSubPath, 3.0f);
}

WLayerAction::WLayerAction(const WActionContext& context, const char* szName, WLayerAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = const_cast<WScene2Document*>(static_cast<const WScene2Document*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      SetIconPath(":/GuiFoundation/Icons/Add.svg");
      break;
    case ActionType::DeleteLayer:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
    case ActionType::SaveLayer:
    case ActionType::SaveActiveLayer:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case ActionType::LayerLoaded:
      SetCheckable(true);
      break;
    case ActionType::LayerVisible:
      SetCheckable(true);
      break;
    case ActionType::SwitchOnSelection:
      SetCheckable(true);
      SetIconPath(":/GuiFoundation/Icons/Cursor.svg");
      break;
  }

  UpdateEnableState();
  m_pSceneDocument->m_LayerEvents.AddEventHandler(WMakeDelegate(&WLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.AddEventHandler(WMakeDelegate(&WLayerAction::DocumentEventHandler, this));
  }
}


WLayerAction::~WLayerAction()
{
  m_pSceneDocument->m_LayerEvents.RemoveEventHandler(WMakeDelegate(&WLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.RemoveEventHandler(WMakeDelegate(&WLayerAction::DocumentEventHandler, this));
  }
}

void WLayerAction::ToggleLayerLoaded(WScene2Document* pSceneDocument, WUuid layerGuid)
{
  bool bLoad = !pSceneDocument->IsLayerLoaded(layerGuid);
  if (!bLoad)
  {
    WSceneDocument* pLayer = pSceneDocument->GetLayerDocument(layerGuid);
    if (pLayer && pLayer->IsModified())
    {
      WStringBuilder sMsg;
      WStringBuilder sLayerName = "<Unknown>";
      {
        const WAssetCurator::WLockedSubAsset subAsset = WAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
        if (subAsset.isValid())
        {
          sLayerName = subAsset->GetName();
        }
      }
      sMsg.SetFormat("The layer '{}' has been modified.\nSave before unloading?", sLayerName);
      QMessageBox::StandardButton res = WQtUiServices::MessageBoxQuestion(sMsg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes);
      switch (res)
      {
        case QMessageBox::Yes:
        {
          WStatus saveRes = pLayer->SaveDocument();
          if (saveRes.Failed())
          {
            saveRes.LogFailure();
            return;
          }
        }
        break;
        case QMessageBox::Cancel:
          return;
        case QMessageBox::Default:
          break;
        default:
          break;
      }
    }
  }

  pSceneDocument->SetLayerLoaded(layerGuid, bLoad).LogFailure();

  if (bLoad)
  {
    pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
  }
}

void WLayerAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CreateLayer:
    {
      WUuid layerGuid;
      QString name = QInputDialog::getText(GetContext().m_pWindow, "Add Layer", "Layer Name:");
      name = name.trimmed();
      if (name.isEmpty())
        return;
      WStatus res = m_pSceneDocument->CreateLayer(name.toUtf8().data(), layerGuid);
      res.LogFailure();
      return;
    }
    case ActionType::DeleteLayer:
    {
      WUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->DeleteLayer(layerGuid).LogFailure();
      return;
    }
    case ActionType::SaveLayer:
    {
      WUuid layerGuid = GetCurrentSelectedLayer();
      if (WSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      WUuid layerGuid = m_pSceneDocument->GetActiveLayer();
      if (WSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::LayerLoaded:
    {
      WUuid layerGuid = GetCurrentSelectedLayer();
      ToggleLayerLoaded(m_pSceneDocument, layerGuid);
      return;
    }
    case ActionType::LayerVisible:
    {
      WUuid layerGuid = GetCurrentSelectedLayer();
      bool bVisible = !m_pSceneDocument->IsLayerVisible(layerGuid);
      m_pSceneDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
      return;
    }
    case ActionType::SwitchOnSelection:
    {
      m_pSceneDocument->SetSwitchLayerToSelection(!m_pSceneDocument->GetSwitchLayerToSelection());
      return;
    }
  }
}

void WLayerAction::LayerEventHandler(const WScene2LayerEvent& e)
{
  UpdateEnableState();
}

void WLayerAction::DocumentEventHandler(const WDocumentEvent& e)
{
  UpdateEnableState();
}

void WLayerAction::UpdateEnableState()
{
  WUuid layerGuid = GetCurrentSelectedLayer();

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      return;
    case ActionType::SwitchOnSelection:
    {
      SetChecked(m_pSceneDocument->GetSwitchLayerToSelection());

      if (m_pSceneDocument->GetSwitchLayerToSelection())
        SetIconPath(":/EditorPluginScene/Icons/SelectAllowed.svg");
      else
        SetIconPath(":/EditorPluginScene/Icons/SelectForbidden.svg");

      TriggerUpdate();
      return;
    }
    case ActionType::DeleteLayer:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      return;
    }
    case ActionType::SaveLayer:
    {
      WSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid);
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      WSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(m_pSceneDocument->GetActiveLayer());
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::LayerLoaded:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(m_pSceneDocument->IsLayerLoaded(layerGuid));
      return;
    }
    case ActionType::LayerVisible:
    {
      SetEnabled(layerGuid.IsValid());
      SetChecked(m_pSceneDocument->IsLayerVisible(layerGuid));
      return;
    }
  }
}

WUuid WLayerAction::GetCurrentSelectedLayer() const
{
  WSelectionManager* pSelection = m_pSceneDocument->GetLayerSelectionManager();
  WUuid layerGuid;
  if (const WDocumentObject* pObject = pSelection->GetCurrentObject())
  {
    WObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
    if (pObject->GetType()->IsDerivedFrom(WGetStaticRTTI<WSceneLayer>()))
    {
      layerGuid = pAccessor->GetByName<WUuid>(pObject, "Layer");
    }
  }
  return layerGuid;
}
