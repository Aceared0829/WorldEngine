#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

WActionDescriptorHandle WStandardMenus::s_hMenuProject;
WActionDescriptorHandle WStandardMenus::s_hMenuFile;
WActionDescriptorHandle WStandardMenus::s_hMenuEdit;
WActionDescriptorHandle WStandardMenus::s_hMenuPanels;
WActionDescriptorHandle WStandardMenus::s_hMenuPanelsAll;
WActionDescriptorHandle WStandardMenus::s_hMenuScene;
WActionDescriptorHandle WStandardMenus::s_hMenuAsset;
WActionDescriptorHandle WStandardMenus::s_hMenuView;
WActionDescriptorHandle WStandardMenus::s_hMenuTools;
WActionDescriptorHandle WStandardMenus::s_hMenuHelp;
WActionDescriptorHandle WStandardMenus::s_hCheckForUpdates;
WActionDescriptorHandle WStandardMenus::s_hReportProblem;
WActionDescriptorHandle WStandardMenus::s_hAskQuestion;

void WStandardMenus::RegisterActions()
{
  s_hMenuProject = W_REGISTER_MENU("G.Project");
  s_hMenuFile = W_REGISTER_MENU("G.File");
  s_hMenuEdit = W_REGISTER_MENU("G.Edit");
  s_hMenuPanels = W_REGISTER_MENU("G.Panels");
  s_hMenuPanelsAll = W_REGISTER_DYNAMIC_MENU("Panels.All", WApplicationPanelsMenuAction, "Show Panels");
  s_hMenuScene = W_REGISTER_MENU("G.Scene");
  s_hMenuAsset = W_REGISTER_MENU("G.Asset");
  s_hMenuView = W_REGISTER_MENU("G.View");
  s_hMenuTools = W_REGISTER_MENU("G.Tools");
  s_hMenuHelp = W_REGISTER_MENU("G.Help");
  s_hCheckForUpdates = W_REGISTER_ACTION_1("Help.CheckForUpdates", WActionScope::Global, "Help", "", WHelpActions, WHelpActions::ButtonType::CheckForUpdates);
  s_hReportProblem = W_REGISTER_ACTION_1("Help.ReportProblem", WActionScope::Global, "Help", "", WHelpActions, WHelpActions::ButtonType::ReportProblem);
  s_hAskQuestion = W_REGISTER_ACTION_1("Help.AskQuestion", WActionScope::Global, "Help", "", WHelpActions, WHelpActions::ButtonType::AskQuestion);
}

void WStandardMenus::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hMenuProject);
  WActionManager::UnregisterAction(s_hMenuFile);
  WActionManager::UnregisterAction(s_hMenuEdit);
  WActionManager::UnregisterAction(s_hMenuPanels);
  WActionManager::UnregisterAction(s_hMenuPanelsAll);
  WActionManager::UnregisterAction(s_hMenuScene);
  WActionManager::UnregisterAction(s_hMenuAsset);
  WActionManager::UnregisterAction(s_hMenuView);
  WActionManager::UnregisterAction(s_hMenuTools);
  WActionManager::UnregisterAction(s_hMenuHelp);
  WActionManager::UnregisterAction(s_hCheckForUpdates);
  WActionManager::UnregisterAction(s_hReportProblem);
  WActionManager::UnregisterAction(s_hAskQuestion);
}

void WStandardMenus::MapActions(WStringView sMapping, const WBitflags<WStandardMenuTypes>& menus)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", sMapping);

  WActionMapDescriptor md;

  if (menus.IsAnySet(WStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", -10000.0f);

  if (menus.IsAnySet(WStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (menus.IsAnySet(WStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (menus.IsAnySet(WStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 3.0f);

  if (menus.IsAnySet(WStandardMenuTypes::Asset))
    pMap->MapAction(s_hMenuAsset, "", 4.0f);

  if (menus.IsAnySet(WStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 5.0f);

  if (menus.IsAnySet(WStandardMenuTypes::Tools))
    pMap->MapAction(s_hMenuTools, "", 6.0f);

  if (menus.IsAnySet(WStandardMenuTypes::Panels))
  {
    pMap->MapAction(s_hMenuPanels, "", 7.0f);
    pMap->MapAction(s_hMenuPanelsAll, "G.Panels", 1.0f);
  }

  if (menus.IsAnySet(WStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hMenuHelp, "", 8.0f);
    pMap->MapAction(s_hReportProblem, "G.Help", 3.0f);
    pMap->MapAction(s_hAskQuestion, "G.Help", 4.0f);
    pMap->MapAction(s_hCheckForUpdates, "G.Help", 10.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// WApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WApplicationPanelsMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct WComparePanels
{
  /// Returns true if a is less than b
  W_ALWAYS_INLINE bool Less(const WDynamicMenuAction::Item& p1, const WDynamicMenuAction::Item& p2) const { return p1.m_sDisplay < p2.m_sDisplay; }

  /// Returns true if a is equal to b
  W_ALWAYS_INLINE bool Equal(const WDynamicMenuAction::Item& p1, const WDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void WApplicationPanelsMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  for (auto* pPanel : WQtApplicationPanel::GetAllApplicationPanels())
  {
    WDynamicMenuAction::Item item;
    item.m_sDisplay = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue = pPanel;
    item.m_Icon = pPanel->icon();
    item.m_CheckState = pPanel->isClosed() ? WDynamicMenuAction::Item::CheckMark::Unchecked : WDynamicMenuAction::Item::CheckMark::Checked;

    out_entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  WComparePanels cp;
  out_entries.Sort<WComparePanels>(cp);
}

void WApplicationPanelsMenuAction::Execute(const WVariant& value)
{
  WQtApplicationPanel* pPanel = static_cast<WQtApplicationPanel*>(value.ConvertTo<void*>());
  if (pPanel->isClosed())
  {
    pPanel->toggleView(true);
    pPanel->EnsureVisible();
  }
  else
  {
    pPanel->toggleView(false);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WHelpActions, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WHelpActions::WHelpActions(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  if (button == ButtonType::ReportProblem)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub.svg");
  }
  if (button == ButtonType::AskQuestion)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub.svg");
  }
}

WHelpActions::~WHelpActions() = default;

void WHelpActions::Execute(const WVariant& value)
{
  if (m_ButtonType == ButtonType::ReportProblem)
  {
    QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/issues"));
  }
  else if (m_ButtonType == ButtonType::AskQuestion)
  {
    QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/discussions"));
  }
  else if (m_ButtonType == ButtonType::CheckForUpdates)
  {
    WQtUiServices::CheckForUpdates();
  }
}
