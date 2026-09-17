#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/WindowLayoutActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <QInputDialog>
#include <QSettings>
#include <ads/DockManager.h>

WActionDescriptorHandle WWindowLayoutActions::s_hCatWindowLayout;
WActionDescriptorHandle WWindowLayoutActions::s_hSetToDefaultPinned;
WActionDescriptorHandle WWindowLayoutActions::s_hSetToDefaultUnpinned;
WActionDescriptorHandle WWindowLayoutActions::s_hSetToAbBottom;
WActionDescriptorHandle WWindowLayoutActions::s_hSaveLayout;
WActionDescriptorHandle WWindowLayoutActions::s_hLoadLayout;

static const char* s_szDefaultLayoutName = "Default";
static const char* s_szSettingsGroup = "EditorWindowLayouts";
static const int s_iNumUserLayoutSlots = 3;

static QByteArray s_DefaultLayoutState;

void WWindowLayoutActions::RegisterActions()
{
  s_hCatWindowLayout = W_REGISTER_MENU("WindowLayout");
  s_hSetToDefaultPinned = W_REGISTER_ACTION_1("Layout.DefaultPinned", WActionScope::Global, "Layout", "", WWindowLayoutAction, WWindowLayoutAction::ButtonType::SetToDefaultPinned);
  s_hSetToDefaultUnpinned = W_REGISTER_ACTION_1("Layout.DefaultUnpinned", WActionScope::Global, "Layout", "", WWindowLayoutAction, WWindowLayoutAction::ButtonType::SetToDefaultUnpinned);
  s_hSetToAbBottom = W_REGISTER_ACTION_1("Layout.AbBottom", WActionScope::Global, "Layout", "", WWindowLayoutAction, WWindowLayoutAction::ButtonType::SetToAbBottom);
  s_hSaveLayout = W_REGISTER_DYNAMIC_MENU("Layout.SaveLayout", WSaveLayoutMenuAction, "Save Layout");
  s_hLoadLayout = W_REGISTER_DYNAMIC_MENU("Layout.LoadLayout", WLoadLayoutMenuAction, "Load Layout");
}

void WWindowLayoutActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCatWindowLayout);
  WActionManager::UnregisterAction(s_hSetToDefaultPinned);
  WActionManager::UnregisterAction(s_hSetToDefaultUnpinned);
  WActionManager::UnregisterAction(s_hSetToAbBottom);
  WActionManager::UnregisterAction(s_hSaveLayout);
  WActionManager::UnregisterAction(s_hLoadLayout);
}

void WWindowLayoutActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCatWindowLayout, "G.Panels", 2.0f);
  pMap->MapAction(s_hSetToDefaultPinned, "WindowLayout", 1.0f);
  pMap->MapAction(s_hSetToDefaultUnpinned, "WindowLayout", 2.0f);
  pMap->MapAction(s_hSetToAbBottom, "WindowLayout", 3.0f);
  pMap->MapAction(s_hSaveLayout, "G.Panels", 3.0f);
  pMap->MapAction(s_hLoadLayout, "G.Panels", 4.0f);
}

void WWindowLayoutActions::RestoreUserLayout()
{
  WQtContainerWindow* pContainer = WQtContainerWindow::GetContainerWindow();
  if (pContainer == nullptr)
    return;

  ads::CDockManager* pDockManager = pContainer->GetDockManager();
  if (pDockManager == nullptr)
    return;

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);
  pDockManager->loadPerspectives(settings);
  settings.endGroup();

  pDockManager->openPerspective(s_szDefaultLayoutName);
}

void WWindowLayoutActions::SaveUserLayout()
{
  WQtContainerWindow* pContainer = WQtContainerWindow::GetContainerWindow();
  if (pContainer == nullptr)
    return;

  ads::CDockManager* pDockManager = pContainer->GetDockManager();
  if (pDockManager == nullptr)
    return;

  pDockManager->addPerspective(s_szDefaultLayoutName);

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);
  pDockManager->savePerspectives(settings);
  settings.endGroup();
}

//////////////////////////////////////////////////////////////////////////
// WWindowLayoutAction
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WWindowLayoutAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WWindowLayoutAction::WWindowLayoutAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case ButtonType::SetToDefaultPinned:
    case ButtonType::SetToDefaultUnpinned:
    case ButtonType::SetToAbBottom:
      break;
  }
}

void WWindowLayoutAction::Execute(const WVariant& value)
{
  WQtContainerWindow* pContainer = WQtContainerWindow::GetContainerWindow();
  if (pContainer == nullptr)
    return;

  ads::CDockManager* pDockManager = pContainer->GetDockManager();
  if (pDockManager == nullptr)
    return;

  // Get all application panels
  const WDynamicArray<WQtApplicationPanel*>& allPanels = WQtApplicationPanel::GetAllApplicationPanels();

  // Find the specific panels we need
  WQtApplicationPanel* pAssetBrowserPanel = WQtAssetBrowserPanel::GetSingleton();
  WQtApplicationPanel* pAssetCuratorPanel = WQtAssetCuratorPanel::GetSingleton();
  WQtApplicationPanel* pLogPanel = WQtLogPanel::GetSingleton();
  WQtApplicationPanel* pCVarPanel = WQtCVarPanel::GetSingleton();
  WQtApplicationPanel* pLongOpsPanel = WQtLongOpsPanel::GetSingleton();

  switch (m_ButtonType)
  {
    case ButtonType::SetToDefaultPinned:
    {
      // Arrange panels the same way as in WQtEditorApp::CreatePanels()
      if (pAssetBrowserPanel)
      {
        pDockManager->removeDockWidget(pAssetBrowserPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
      }
      if (pAssetCuratorPanel)
      {
        pDockManager->removeDockWidget(pAssetCuratorPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
      }
      if (pLogPanel)
      {
        pDockManager->removeDockWidget(pLogPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
      }
      if (pCVarPanel)
      {
        pDockManager->removeDockWidget(pCVarPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
      }
      if (pLongOpsPanel)
      {
        pDockManager->removeDockWidget(pLongOpsPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);
      }

      break;
    }

    case ButtonType::SetToDefaultUnpinned:
    {
      // Arrange panels the same way as in WQtEditorApp::CreatePanels()
      // but set them all to unpinned (auto hide)
      if (pAssetBrowserPanel)
      {
        pDockManager->removeDockWidget(pAssetBrowserPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
        pAssetBrowserPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pAssetCuratorPanel)
      {
        pDockManager->removeDockWidget(pAssetCuratorPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
        pAssetCuratorPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pLogPanel)
      {
        pDockManager->removeDockWidget(pLogPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
        pLogPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pCVarPanel)
      {
        pDockManager->removeDockWidget(pCVarPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
        pCVarPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pLongOpsPanel)
      {
        pDockManager->removeDockWidget(pLongOpsPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);
        pLongOpsPanel->setAutoHide(true, ads::SideBarRight);
      }
      break;
    }

    case ButtonType::SetToAbBottom:
    {
      // Arrange most panels as in WQtEditorApp::CreatePanels()
      // but dock the asset browser at the bottom of the screen (pinned / not auto-hide)
      if (pAssetBrowserPanel)
      {
        pDockManager->removeDockWidget(pAssetBrowserPanel);
        pDockManager->addDockWidget(ads::BottomDockWidgetArea, pAssetBrowserPanel);
      }
      if (pAssetCuratorPanel)
      {
        pDockManager->removeDockWidget(pAssetCuratorPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
        pAssetCuratorPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pLogPanel)
      {
        pDockManager->removeDockWidget(pLogPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
        pLogPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pCVarPanel)
      {
        pDockManager->removeDockWidget(pCVarPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
        pCVarPanel->setAutoHide(true, ads::SideBarRight);
      }
      if (pLongOpsPanel)
      {
        pDockManager->removeDockWidget(pLongOpsPanel);
        pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);
        pLongOpsPanel->setAutoHide(true, ads::SideBarRight);
      }

      break;
    }
  }

  // Hide panels that are hidden by default
  if (pCVarPanel)
  {
    pCVarPanel->toggleView(false);
  }

  if (pLongOpsPanel)
  {
    pLongOpsPanel->toggleView(false);
  }

  // Raise the asset browser to front
  if (pAssetBrowserPanel)
  {
    pAssetBrowserPanel->EnsureVisible();
  }
}

//////////////////////////////////////////////////////////////////////////
// WSaveLayoutMenuAction
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSaveLayoutMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSaveLayoutMenuAction::WSaveLayoutMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
}

void WSaveLayoutMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);

  WStringBuilder sDisplay;
  for (int i = 0; i < s_iNumUserLayoutSlots; ++i)
  {
    const QString sNameKey = QString("UserSlot_%1_Name").arg(i);
    const QString sSlotName = settings.value(sNameKey, QString()).toString();

    Item item;
    if (sSlotName.isEmpty())
    {
      sDisplay.SetFormat("Slot {0}: [Empty]", i + 1);
    }
    else
    {
      sDisplay.SetFormat("Slot {0}: {1}", i + 1, sSlotName.toUtf8().constData());
    }
    item.m_sDisplay = sDisplay;
    item.m_UserValue = i;
    out_entries.PushBack(item);
  }

  settings.endGroup();
}

void WSaveLayoutMenuAction::Execute(const WVariant& value)
{
  WQtContainerWindow* pContainer = WQtContainerWindow::GetContainerWindow();
  if (pContainer == nullptr)
    return;

  ads::CDockManager* pDockManager = pContainer->GetDockManager();
  if (pDockManager == nullptr)
    return;

  const int iSlot = value.ConvertTo<int>();

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);

  const QString sNameKey = QString("UserSlot_%1_Name").arg(iSlot);
  const QString sStateKey = QString("UserSlot_%1_State").arg(iSlot);
  const QString sExistingName = settings.value(sNameKey, QString()).toString();

  // native window, so not covered by WQtDialog
  if (WQtUiServices::SuppressModalWindow("Save Layout (name prompt)"))
  {
    settings.endGroup();
    return;
  }

  bool bOk = false;
  const QString sName = QInputDialog::getText(pContainer, QStringLiteral("Save Layout"), QStringLiteral("Layout name:"), QLineEdit::Normal, sExistingName, &bOk);

  if (!bOk || sName.trimmed().isEmpty())
  {
    settings.endGroup();
    return;
  }

  settings.setValue(sNameKey, sName.trimmed());
  settings.setValue(sStateKey, pDockManager->saveState());

  settings.endGroup();
}

//////////////////////////////////////////////////////////////////////////
// WLoadLayoutMenuAction
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLoadLayoutMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WLoadLayoutMenuAction::WLoadLayoutMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
}

void WLoadLayoutMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);

  WStringBuilder sDisplay;
  for (int i = 0; i < s_iNumUserLayoutSlots; ++i)
  {
    const QString sNameKey = QString("UserSlot_%1_Name").arg(i);
    const QString sSlotName = settings.value(sNameKey, QString()).toString();

    if (sSlotName.isEmpty())
      continue;

    Item item;
    sDisplay.SetFormat("Slot {0}: {1}", i + 1, sSlotName.toUtf8().constData());
    item.m_sDisplay = sDisplay;
    item.m_UserValue = i;
    out_entries.PushBack(item);
  }

  settings.endGroup();
}

void WLoadLayoutMenuAction::Execute(const WVariant& value)
{
  WQtContainerWindow* pContainer = WQtContainerWindow::GetContainerWindow();
  if (pContainer == nullptr)
    return;

  ads::CDockManager* pDockManager = pContainer->GetDockManager();
  if (pDockManager == nullptr)
    return;

  const int iSlot = value.ConvertTo<int>();

  QSettings settings;
  settings.beginGroup(s_szSettingsGroup);

  const QString sNameKey = QString("UserSlot_%1_Name").arg(iSlot);
  const QString sStateKey = QString("UserSlot_%1_State").arg(iSlot);

  if (settings.value(sNameKey, QString()).toString().isEmpty())
  {
    settings.endGroup();
    return;
  }

  const QByteArray state = settings.value(sStateKey).toByteArray();
  settings.endGroup();

  if (!state.isEmpty())
  {
    pDockManager->restoreState(state);
  }
}
