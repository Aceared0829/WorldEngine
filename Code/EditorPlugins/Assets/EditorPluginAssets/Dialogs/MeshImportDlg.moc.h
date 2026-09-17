#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_MeshImportDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WMeshImportDlg : public WQtDialog, public Ui_MeshImportDlg
{
  Q_OBJECT

public:
  WMeshImportDlg(QWidget* pParent);

  WString m_sTitle;
  WString m_sSharedMaterialsFolderAbs;
  bool m_bApplyToAll = false;
  bool m_bUseSharedMaterials = false;
  bool m_bCreateMaterials = true;
  bool m_bShowAnimMeshOptions = false;
  bool m_bReuseExistingSkeleton = false;
  bool m_bImportAnimationClips = false;
  bool m_bAddLODs = false;
  WUInt8 m_uiNumLODs = 1;
  WUuid m_SharedSkeleton;
  WString m_sMeshLodPrefix;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_BrowseMaterialsFolder_clicked();
  void on_BrowseSkeleton_clicked();
  void on_UseSharedMaterials_clicked(bool);
  void on_Materials_clicked(bool);
  void on_ReuseSkeleton_clicked(bool);
  void on_Help_clicked(bool);

private:
  virtual void showEvent(QShowEvent*) override;

  void UpdateUI();

  WString m_sSharedSkeleton;
};
