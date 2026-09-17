#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class W_EDITORFRAMEWORK_DLL WQtAssetBrowserDlg : public WQtDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  WQtAssetBrowserDlg(QWidget* pParent, const WUuid& preselectedAsset, WStringView sVisibleFilters, WStringView sWindowTitle = {}, WStringView sRequiredTag = {});
  WQtAssetBrowserDlg(QWidget* pParent, WStringView sWindowTitle, WStringView sPreselectedFileAbs, WStringView sFileExtensions);
  ~WQtAssetBrowserDlg();

  WStringView GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  WStringView GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const WUuid GetSelectedAssetGuid() const { return m_SelectedAssetGuid; }

private Q_SLOTS:
  void on_AssetBrowserWidget_ItemChosen(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemSelected(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemCleared();
  void on_ButtonSelect_clicked();

private:
  void Init(QWidget* pParent);

  WString m_sSelectedAssetPathRelative;
  WString m_sSelectedAssetPathAbsolute;
  WUuid m_SelectedAssetGuid;
  WString m_sVisibleFilters;
  WString m_sRequiredTag;

  static bool s_bShowItemsInSubFolder;
  static bool s_bShowItemsInHiddenFolder;
  static bool s_bSortByRecentUse;
  static WMap<WString, WString> s_TextFilter;
  static WMap<WString, WString> s_PathFilter;
  static WMap<WString, WString> s_TypeFilter;
};
