#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <QDesktopServices>

W_IMPLEMENT_SINGLETON(WQtSettingsTab);

WString WQtSettingsTab::GetWindowIcon() const
{
  return ""; //:/GuiFoundation/W-logo.svg";
}

WString WQtSettingsTab::GetDisplayNameShort() const
{
  return "Settings";
}

void WQtEditorApp::ShowSettingsDocument()
{
  W_PROFILE_SCOPE("ShowSettingsDocument");
  WQtSettingsTab* pSettingsTab = WQtSettingsTab::GetSingleton();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new WQtSettingsTab();
  }
}

void WQtEditorApp::CloseSettingsDocument()
{
  WQtSettingsTab* pSettingsTab = WQtSettingsTab::GetSingleton();

  if (pSettingsTab != nullptr)
  {
    pSettingsTab->CloseDocumentWindow();
  }
}

WQtSettingsTab::WQtSettingsTab()
  : WQtDocumentWindow("Settings")
  , m_SingletonRegistrar(this)
{
  WQtMenuBarActionMapView* pMenuBar = static_cast<WQtMenuBarActionMapView*>(menuBar());
  WActionContext context;
  context.m_sMapping = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  FinishWindowCreation();
}

WQtSettingsTab::~WQtSettingsTab() = default;

bool WQtSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return WQtDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void WQtSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  UnregisterSingleton();
}
