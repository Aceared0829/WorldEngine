#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

W_IMPLEMENT_SINGLETON(WQtAssetBrowserPanel);

WQtAssetBrowserPanel::WQtAssetBrowserPanel(ads::CDockManager* pDockManager)
  : WQtApplicationPanel(pDockManager, "Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setFeature(ads::CDockWidget::DockWidgetClosable, false);

  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(pDummy);

  setIcon(WQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Asset.svg"));
  setWindowTitle(WMakeQString(WTranslate("Panel.AssetBrowser")));

  W_VERIFY(connect(AssetBrowserWidget, &WQtAssetBrowserWidget::ItemChosen, this, &WQtAssetBrowserPanel::SlotAssetChosen) != nullptr,
    "signal/slot connection failed");
  W_VERIFY(connect(AssetBrowserWidget, &WQtAssetBrowserWidget::ItemSelected, this, &WQtAssetBrowserPanel::SlotAssetSelected) != nullptr,
    "signal/slot connection failed");
  W_VERIFY(connect(AssetBrowserWidget, &WQtAssetBrowserWidget::ItemCleared, this, &WQtAssetBrowserPanel::SlotAssetCleared) != nullptr,
    "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
}

WQtAssetBrowserPanel::~WQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void WQtAssetBrowserPanel::SlotAssetChosen(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags)
{
  if (guid.IsValid())
  {
    WQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
  }
  else
  {
    WQtUiServices::OpenFileInDefaultProgram(qtToEzString(sAssetPathAbsolute)).IgnoreResult();
  }
}

void WQtAssetBrowserPanel::SlotAssetSelected(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags)
{
  m_LastSelected = guid;
}

void WQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected = WUuid::MakeInvalid();
}
