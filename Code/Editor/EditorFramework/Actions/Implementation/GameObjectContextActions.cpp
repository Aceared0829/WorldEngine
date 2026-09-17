#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectContextAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WGameObjectContextActions::s_hCategory;
WActionDescriptorHandle WGameObjectContextActions::s_hPickContextScene;
WActionDescriptorHandle WGameObjectContextActions::s_hPickContextObject;
WActionDescriptorHandle WGameObjectContextActions::s_hClearContextObject;

void WGameObjectContextActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("GameObjectContextCategory");
  s_hPickContextScene = W_REGISTER_ACTION_1("GameObjectContext.PickContextScene", WActionScope::Window, "Game Object Context", "",
    WGameObjectContextAction, WGameObjectContextAction::ActionType::PickContextScene);
  s_hPickContextObject = W_REGISTER_ACTION_1("GameObjectContext.PickContextObject", WActionScope::Window, "Game Object Context", "",
    WGameObjectContextAction, WGameObjectContextAction::ActionType::PickContextObject);
  s_hClearContextObject = W_REGISTER_ACTION_1("GameObjectContext.ClearContextObject", WActionScope::Window, "Game Object Context", "",
    WGameObjectContextAction, WGameObjectContextAction::ActionType::ClearContextObject);
}


void WGameObjectContextActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hPickContextScene);
  WActionManager::UnregisterAction(s_hPickContextObject);
  WActionManager::UnregisterAction(s_hClearContextObject);
}

void WGameObjectContextActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const WStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextScene, szSubPath, 1.0f);
}


void WGameObjectContextActions::MapContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const WStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextObject, szSubPath, 1.0f);
  pMap->MapAction(s_hClearContextObject, szSubPath, 2.0f);
}

WGameObjectContextAction::WGameObjectContextAction(const WActionContext& context, const char* szName, WGameObjectContextAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::PickContextScene:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::PickContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::ClearContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    default:
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectContextAction::SelectionEventHandler, this));
  Update();
}

WGameObjectContextAction::~WGameObjectContextAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WGameObjectContextAction::SelectionEventHandler, this));
}

void WGameObjectContextAction::Execute(const WVariant& value)
{
  WGameObjectContextDocument* pDocument = static_cast<WGameObjectContextDocument*>(GetContext().m_pDocument);
  WUuid document = pDocument->GetContextDocumentGuid();
  switch (m_Type)
  {
    case ActionType::PickContextScene:
    {
      WQtAssetBrowserDlg dlg(GetContext().m_pWindow, document, "Scene;Prefab");
      if (dlg.exec() == 0)
        return;

      document = dlg.GetSelectedAssetGuid();
      pDocument->SetContext(document, WUuid()).LogFailure();
      return;
    }
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      if (selection.GetCount() == 1)
      {
        if (selection[0]->GetType() == WGetStaticRTTI<WGameObject>())
        {
          pDocument->SetContext(document, selection[0]->GetGuid()).LogFailure();
        }
      }
    }
      return;
    case ActionType::ClearContextObject:
    {
      pDocument->SetContext(document, WUuid()).LogFailure();
    }
      return;
  }
}

void WGameObjectContextAction::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  Update();
}

void WGameObjectContextAction::Update()
{
  WGameObjectContextDocument* pDocument = static_cast<WGameObjectContextDocument*>(GetContext().m_pDocument);

  switch (m_Type)
  {
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      bool bIsSingleGameObject = selection.GetCount() == 1 && selection[0]->GetType() == WGetStaticRTTI<WGameObject>();
      SetEnabled(bIsSingleGameObject);
    }

    default:
      break;
  }
}
