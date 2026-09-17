#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/Dialogs/MeshImportDlg.moc.h>
#include <QFileDialog>

WMeshImportDlg::WMeshImportDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);
}

void WMeshImportDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  CurrentAsset->setText(WMakeQString(m_sTitle));
  Skeleton->setVisible(m_bShowAnimMeshOptions);
  Animations->setVisible(m_bShowAnimMeshOptions);
  ApplyToAll->setChecked(m_bApplyToAll);
  ImportAnimClips->setChecked(m_bImportAnimationClips);
  ReuseSkeleton->setChecked(m_bReuseExistingSkeleton);
  UseSharedMaterials->setChecked(m_bUseSharedMaterials);
  Lod->setChecked(m_bAddLODs);
  NumLODs->setValue(m_uiNumLODs);
  LodIncludeTag->setText(WMakeQString(m_sMeshLodPrefix));

  UpdateUI();
}

void WMeshImportDlg::UpdateUI()
{

  BrowseMaterialsFolder->setEnabled(m_bUseSharedMaterials);
  MaterialsFolder->setEnabled(m_bUseSharedMaterials);
  MaterialsFolder->setReadOnly(true);
  Materials->setChecked(m_bCreateMaterials);
  SkeletonAsset->setEnabled(m_bReuseExistingSkeleton);
  BrowseSkeleton->setEnabled(m_bReuseExistingSkeleton);

  WStringBuilder sRelPath = m_sSharedMaterialsFolderAbs;
  if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelPath))
  {
    MaterialsFolder->setText(WMakeQString(sRelPath));
  }

  bool ok = true;
  if (m_bCreateMaterials && m_bUseSharedMaterials)
  {
    if (!WOSFile::ExistsDirectory(m_sSharedMaterialsFolderAbs))
    {
      ok = false;
    }
  }

  if (!m_bCreateMaterials)
  {
    MaterialsFolder->setText("No material assets will be created.");
  }
  else if (!m_bUseSharedMaterials)
  {
    MaterialsFolder->setText("Creating material assets in '_data' folder besides asset.");
  }

  if (m_bShowAnimMeshOptions)
  {
    if (!m_bReuseExistingSkeleton)
    {
      SkeletonAsset->setText("A new skeleton asset will be created.");
    }
    else
    {
      if (!m_SharedSkeleton.IsValid())
      {
        m_sSharedSkeleton.Clear();
        ok = false;

        SkeletonAsset->setText("Browse for an existing skeleton asset ->");
      }
      else
      {
        if (m_sSharedSkeleton.IsEmpty())
        {
          if (auto pAsset = WAssetCurator::GetSingleton()->GetSubAsset(m_SharedSkeleton))
          {
            m_sSharedSkeleton = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          }
        }

        if (m_sSharedSkeleton.IsEmpty())
        {
          ok = false;
          SkeletonAsset->setText("Invalid skeleton asset.");
        }
        else
        {
          SkeletonAsset->setText(WMakeQString(m_sSharedSkeleton));
        }
      }
    }
  }

  Buttons->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(ok);
}

void WMeshImportDlg::on_Buttons_accepted()
{
  m_bCreateMaterials = Materials->isChecked();
  m_bImportAnimationClips = ImportAnimClips->isChecked();
  m_bApplyToAll = ApplyToAll->isChecked();
  m_bAddLODs = Lod->isChecked();
  m_uiNumLODs = NumLODs->value();
  m_sMeshLodPrefix = LodIncludeTag->text().toUtf8().data();

  accept();
}

void WMeshImportDlg::on_Buttons_rejected()
{
  reject();
}

void WMeshImportDlg::on_BrowseMaterialsFolder_clicked()
{
  WStringBuilder sPath = m_sSharedMaterialsFolderAbs;

  if (sPath.IsEmpty())
  {
    sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  }

  if (!WQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
  {
    sPath.Clear();
  }

  QString sSelectedPath = QFileDialog::getExistingDirectory(this, "Select Folder", WMakeQString(sPath));
  if (sSelectedPath.isEmpty())
    return;

  WStringBuilder sRelPath = sSelectedPath.toUtf8().data();

  if (!WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelPath))
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("The select path isn't in any of the project's data directories.\n\nPlease select another folder.");
    return;
  }

  m_sSharedMaterialsFolderAbs = sSelectedPath.toUtf8().data();

  UpdateUI();
}

void WMeshImportDlg::on_BrowseSkeleton_clicked()
{
  WQtAssetBrowserDlg dlg(this, WUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skeleton", "Select Skeleton");
  if (dlg.exec() != 0)
  {
    if (dlg.GetSelectedAssetGuid().IsValid())
    {
      m_SharedSkeleton = dlg.GetSelectedAssetGuid();
      m_sSharedSkeleton.Clear();
    }

    UpdateUI();
  }
}

void WMeshImportDlg::on_UseSharedMaterials_clicked(bool)
{
  m_bUseSharedMaterials = UseSharedMaterials->isChecked();

  UpdateUI();
}

void WMeshImportDlg::on_Materials_clicked(bool)
{
  m_bCreateMaterials = Materials->isChecked();

  UpdateUI();
}

void WMeshImportDlg::on_ReuseSkeleton_clicked(bool)
{
  m_bReuseExistingSkeleton = ReuseSkeleton->isChecked();

  UpdateUI();
}

void WMeshImportDlg::on_Help_clicked(bool)
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net/pages/docs/graphics/meshes/mesh-import.html"));
}
