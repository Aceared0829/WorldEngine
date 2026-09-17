#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>

namespace
{
  WAssetBrowserSelection s_CurrentAssetBrowserSelection;
} // namespace

const WAssetBrowserSelection& WAssetBrowserSelection::GetCurrent()
{
  return s_CurrentAssetBrowserSelection;
}

void WAssetBrowserSelection::SetCurrent(WAssetBrowserSelection&& selection)
{
  s_CurrentAssetBrowserSelection = std::move(selection);
}

WActionDescriptorHandle WAssetBrowserContextMenu::s_hAssetMenu;

void WAssetBrowserContextMenu::RegisterActions()
{
  s_hAssetMenu = W_REGISTER_MENU_WITH_ICON("AssetBrowser.AssetMenu", ":/GuiFoundation/Icons/Document.svg");
}

void WAssetBrowserContextMenu::MapActions()
{
  WActionMap* pMap = WActionMapManager::GetActionMap("AssetBrowserContextMenu");
  W_ASSERT_DEV(pMap != nullptr, "The action map 'AssetBrowserContextMenu' does not exist.");

  pMap->MapAction(s_hAssetMenu, "", 1.0f);
}

void WAssetBrowserContextMenu::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hAssetMenu);
}
