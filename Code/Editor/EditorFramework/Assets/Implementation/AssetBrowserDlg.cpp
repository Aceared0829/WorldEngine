#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

bool WQtAssetBrowserDlg::s_bShowItemsInSubFolder = true;
bool WQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool WQtAssetBrowserDlg::s_bSortByRecentUse = true;
WMap<WString, WString> WQtAssetBrowserDlg::s_TextFilter;
WMap<WString, WString> WQtAssetBrowserDlg::s_PathFilter;
WMap<WString, WString> WQtAssetBrowserDlg::s_TypeFilter;

void ClampWindowGeometryToScreens(QRect& ref_windowGeometry)
{
  const QList<QScreen*> screens = QGuiApplication::screens();

  for (QScreen* screen : screens)
  {
    const QRect screenGeom = screen->availableGeometry();
    if (screenGeom.intersects(ref_windowGeometry))
      return;
  }

  const QRect primaryGeom = QGuiApplication::primaryScreen()->availableGeometry();

  const QSize size = ref_windowGeometry.size();
  ref_windowGeometry.setLeft(WMath::Clamp(ref_windowGeometry.left(), primaryGeom.left(), primaryGeom.right() - ref_windowGeometry.width()));
  ref_windowGeometry.setTop(WMath::Clamp(ref_windowGeometry.top(), primaryGeom.top(), primaryGeom.bottom() - ref_windowGeometry.height()));
  ref_windowGeometry.setSize(size);
}

void WQtAssetBrowserDlg::Init(QWidget* pParent)
{
  setupUi(this);

  ButtonSelect->setEnabled(false);

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

    QRect windowGeometry;
    windowGeometry.setTopLeft(Settings.value("WindowPosition", pos()).toPoint());
    windowGeometry.setSize(Settings.value("WindowSize", size()).toSize());
    ClampWindowGeometryToScreens(windowGeometry);

    move(windowGeometry.topLeft());
    resize(windowGeometry.size());
  }
  Settings.endGroup();

  AssetBrowserWidget->RestoreState("AssetBrowserDlg");
  AssetBrowserWidget->GetAssetBrowserFilter()->SetSortByRecentUse(s_bSortByRecentUse);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInSubFolders(s_bShowItemsInSubFolder);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInHiddenFolders(s_bShowItemsInHiddenFolder);

  if (!s_TextFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTextFilter(s_TextFilter[m_sVisibleFilters]);

  if (!s_PathFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetPathFilter(s_PathFilter[m_sVisibleFilters]);

  if (!s_TypeFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTypeFilter(s_TypeFilter[m_sVisibleFilters]);
}

WQtAssetBrowserDlg::WQtAssetBrowserDlg(QWidget* pParent, const WUuid& preselectedAsset, WStringView sVisibleFilters, WStringView sWindowTitle, WStringView sRequiredTag)
  : WQtDialog(pParent)
{
  {
    WStringBuilder temp = sVisibleFilters;
    WTempHybridArray<WStringView, 4> compTypes;
    temp.Split(false, compTypes, ";");
    WStringBuilder allFiltered = sVisibleFilters;

    for (const auto& descIt : WAssetDocumentManager::GetAllDocumentDescriptors())
    {
      const WDocumentTypeDescriptor* pType = descIt.Value();
      for (WStringView ct : compTypes)
      {
        if (pType->m_CompatibleTypes.Contains(ct))
        {
          allFiltered.Append(";", pType->m_sDocumentTypeName, ";");
        }
      }
    }

    m_sVisibleFilters = allFiltered;
    m_sRequiredTag = sRequiredTag;
  }
  Init(pParent);

  AssetBrowserWidget->SetMode(WQtAssetBrowserWidget::Mode::AssetPicker);

  if (m_sVisibleFilters != ";;") // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(m_sVisibleFilters);
  }

  AssetBrowserWidget->SetRequiredTag(m_sRequiredTag);

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  AssetBrowserWidget->SearchWidget->setFocus();

  if (!sWindowTitle.IsEmpty())
  {
    setWindowTitle(WMakeQString(sWindowTitle));
  }
}

WQtAssetBrowserDlg::WQtAssetBrowserDlg(QWidget* pParent, WStringView sWindowTitle, WStringView sPreselectedFileAbs, WStringView sFileExtensions)
  : WQtDialog(pParent)
{
  m_sVisibleFilters = sFileExtensions;

  Init(pParent);

  WStringBuilder title(sFileExtensions, ")");
  title.ReplaceAll(";", "; ");
  title.ReplaceAll("  ", " ");
  title.PrependFormat("{} (", sWindowTitle);
  setWindowTitle(WMakeQString(title));

  AssetBrowserWidget->SetMode(WQtAssetBrowserWidget::Mode::FilePicker);
  AssetBrowserWidget->UseFileExtensionFilters(sFileExtensions);

  WStringBuilder sParentRelPath = sPreselectedFileAbs;
  if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sParentRelPath))
  {
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTemporaryPinnedItem(sParentRelPath);
  }

  AssetBrowserWidget->SetSelectedFile(sPreselectedFileAbs);

  AssetBrowserWidget->SearchWidget->setFocus();
}

WQtAssetBrowserDlg::~WQtAssetBrowserDlg()
{
  s_bShowItemsInSubFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInSubFolders();
  s_bShowItemsInHiddenFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInHiddenFolders();
  s_bSortByRecentUse = AssetBrowserWidget->GetAssetBrowserFilter()->GetSortByRecentUse();
  s_TextFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTextFilter();
  s_PathFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetPathFilter();
  s_TypeFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTypeFilter();

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }
  Settings.endGroup();

  AssetBrowserWidget->SaveState("AssetBrowserDlg");
}

void WQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  const WBitflags<WAssetBrowserItemFlags> flags = (WAssetBrowserItemFlags::Enum)uiAssetBrowserItemFlags;

  ButtonSelect->setEnabled(flags.IsAnySet(WAssetBrowserItemFlags::Asset | WAssetBrowserItemFlags::SubAsset | WAssetBrowserItemFlags::File));
}

void WQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  accept();
}

void WQtAssetBrowserDlg::on_AssetBrowserWidget_ItemCleared()
{
  ButtonSelect->setEnabled(false);
}

void WQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  accept();
}
