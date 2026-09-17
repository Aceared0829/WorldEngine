#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginJolt/Dialogs/CreateMeshColliderDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>
#include <QPushButton>
#include <ToolsFoundation/Project/ToolsProject.h>

bool WQtCreateMeshColliderDlg::s_bOpenAfterCreate = true;
WEnum<WMeshColliderKind> WQtCreateMeshColliderDlg::s_LastKind = WMeshColliderKind::Default;

WQtCreateMeshColliderDlg::WQtCreateMeshColliderDlg(const WMeshColliderSource& source, QWidget* pParent)
  : WQtDialog(pParent)
  , m_pSource(&source)
{
  Setup();

  UpdateSuggestedPath();
  UpdateInfo();
}

WQtCreateMeshColliderDlg::WQtCreateMeshColliderDlg(WUInt32 uiMeshCount, QWidget* pParent)
  : WQtDialog(pParent)
  , m_uiMeshCount(uiMeshCount)
{
  Setup();

  // with several meshes there is no one path to pick, each collider goes next to its own mesh
  ColliderPath->setEnabled(false);
  BrowseButton->setEnabled(false);
  m_bSettingPath = true;
  ColliderPath->setText(QLatin1String("<next to each mesh asset>"));
  m_bSettingPath = false;

  // the box stays available, it is only unticked by default for a large selection
  OpenAfterCreate->setText(QString("Open all %1 collision meshes after creating them").arg(uiMeshCount));
  OpenAfterCreate->setChecked(s_bOpenAfterCreate && uiMeshCount <= s_uiMaxAutoOpen);

  UpdateInfo();
}

void WQtCreateMeshColliderDlg::Setup()
{
  setupUi(this);

  ColliderType->addItem("Convex Hull", QVariant((int)WMeshColliderKind::ConvexHull));
  ColliderType->addItem("Triangle Mesh", QVariant((int)WMeshColliderKind::TriangleMesh));
  ColliderType->setCurrentIndex(ColliderType->findData(QVariant((int)s_LastKind.GetValue())));

  OpenAfterCreate->setChecked(s_bOpenAfterCreate);
}

WEnum<WMeshColliderKind> WQtCreateMeshColliderDlg::GetSelectedKind() const
{
  return (WMeshColliderKind::Enum)ColliderType->currentData().toInt();
}

void WQtCreateMeshColliderDlg::UpdateSuggestedPath()
{
  if (m_pSource == nullptr || !m_bPathIsSuggestion)
    return;

  const WString sPath = WMeshColliderCreator::MakeDisplayPath(
    WMeshColliderCreator::SuggestColliderPath(*m_pSource, GetSelectedKind(), OverwriteExisting->isChecked()));

  // the change is our own, so the text handler must not treat it as the user typing a path
  m_bSettingPath = true;
  ColliderPath->setText(WMakeQString(sPath));
  m_bSettingPath = false;
}

void WQtCreateMeshColliderDlg::UpdateInfo()
{
  // Only a convex mesh has a single surface. A triangle mesh gets one per material slot of the
  // model, filled in by the asset transform.
  const bool bHasSurface = GetSelectedKind() == WMeshColliderKind::ConvexHull;
  labelSurface->setVisible(bHasSurface);
  SurfaceAsset->setVisible(bHasSurface);
  SurfaceButton->setVisible(bHasSurface);
  SurfaceClearButton->setVisible(bHasSurface);

  WStringBuilder sWarning;

  if (m_pSource == nullptr)
  {
    // nothing to validate: the paths are decided per mesh, and every problem with one mesh is a skip
  }
  else if (m_pSource->m_bIsPrimitive)
  {
    sWarning = "This mesh asset uses a procedural primitive, not a model file, so no collision mesh can be generated from it.";
  }
  else if (m_pSource->m_sMeshFile.IsEmpty())
  {
    sWarning = "The source file of this mesh asset could not be read, so no collision mesh can be generated from it.";
  }
  else
  {
    const WString sTyped = qtToEzString(ColliderPath->text());

    WStringBuilder sAbsolute;
    if (sTyped.IsEmpty())
    {
      sWarning = "Enter a path for the collision mesh asset.";
    }
    else if (WMeshColliderCreator::ResolveDisplayPath(sTyped, sAbsolute).Failed())
    {
      sWarning = "This path does not start with the name of a data directory.";
    }
    else if (WOSFile::ExistsFile(sAbsolute) && !OverwriteExisting->isChecked())
    {
      sWarning = "This file already exists. Tick the box below to overwrite it, or choose a different name.";
    }
  }

  WarningLabel->setText(WMakeQString(sWarning));

  if (QPushButton* pOk = ButtonBox->button(QDialogButtonBox::Ok))
  {
    pOk->setEnabled(sWarning.IsEmpty());
  }
}

void WQtCreateMeshColliderDlg::on_ColliderType_currentIndexChanged(int index)
{
  UpdateSuggestedPath();
  UpdateInfo();
}

void WQtCreateMeshColliderDlg::on_OverwriteExisting_toggled(bool checked)
{
  // the suggestion dodges an existing file by appending a number, which overwriting no longer wants
  UpdateSuggestedPath();
  UpdateInfo();
}

void WQtCreateMeshColliderDlg::on_ColliderPath_textChanged(const QString& text)
{
  if (!m_bSettingPath)
  {
    m_bPathIsSuggestion = false;
  }

  UpdateInfo();
}

void WQtCreateMeshColliderDlg::on_BrowseButton_clicked()
{
  const bool bConvex = GetSelectedKind() == WMeshColliderKind::ConvexHull;

  const QString sFilter = bConvex ? QLatin1String("Convex Collision Mesh (*.WJoltConvexCollisionMeshAsset)")
                                  : QLatin1String("Collision Mesh (*.WJoltCollisionMeshAsset)");

  // the file dialog needs a real path, the line edit holds a data directory relative one
  WStringBuilder sStart;
  if (WMeshColliderCreator::ResolveDisplayPath(qtToEzString(ColliderPath->text()), sStart).Failed() && m_pSource != nullptr)
  {
    sStart = WMeshColliderCreator::SuggestColliderPath(*m_pSource, GetSelectedKind());
  }

  QString sFile = QFileDialog::getSaveFileName(this, QLatin1String("Create Collision Mesh"), WMakeQString(sStart), sFilter,
    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  // a path the user browsed to must survive a change of the collider type
  m_bPathIsSuggestion = false;
  ColliderPath->setText(WMakeQString(WMeshColliderCreator::MakeDisplayPath(qtToEzString(sFile))));
}

void WQtCreateMeshColliderDlg::on_SurfaceButton_clicked()
{
  const WUuid current = WConversionUtils::ConvertStringToUuid(m_sSurface);

  WQtAssetBrowserDlg dlg(this, current, "CompatibleAsset_Surface");
  if (dlg.exec() == 0)
    return;

  const WUuid selected = dlg.GetSelectedAssetGuid();
  if (!selected.IsValid())
    return;

  // The guid is what goes into the asset, the path is only shown.
  WStringBuilder sGuid;
  WConversionUtils::ToString(selected, sGuid);
  m_sSurface = sGuid;

  SurfaceAsset->setText(WMakeQString(dlg.GetSelectedAssetPathRelative()));
}

void WQtCreateMeshColliderDlg::on_SurfaceClearButton_clicked()
{
  m_sSurface.Clear();
  SurfaceAsset->clear();
}

void WQtCreateMeshColliderDlg::on_ButtonBox_accepted()
{
  // Left empty for several meshes, which is what makes the creator use each mesh's default path.
  m_Options.m_sColliderPath = (m_pSource != nullptr) ? qtToEzString(ColliderPath->text()) : WString();
  m_Options.m_Kind = GetSelectedKind();
  s_LastKind = m_Options.m_Kind;
  m_Options.m_sSurface = (GetSelectedKind() == WMeshColliderKind::ConvexHull) ? m_sSurface : WString();
  m_Options.m_bOverwriteExisting = OverwriteExisting->isChecked();

  // Not remembered for a large selection, where the box was unticked by us rather than by the user.
  if (m_uiMeshCount <= s_uiMaxAutoOpen)
  {
    s_bOpenAfterCreate = OpenAfterCreate->isChecked();
  }

  m_Options.m_bOpenAfterCreate = OpenAfterCreate->isChecked();

  accept();
}

void WQtCreateMeshColliderDlg::on_ButtonBox_rejected()
{
  reject();
}
