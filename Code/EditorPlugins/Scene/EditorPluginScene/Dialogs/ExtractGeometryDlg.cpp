#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

#include <QFileDialog>

QString WQtExtractGeometryDlg::s_sDestinationFile;
bool WQtExtractGeometryDlg::s_bOnlySelection = false;
int WQtExtractGeometryDlg::s_iExtractionMode = (int)WWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
int WQtExtractGeometryDlg::s_iCoordinateSystem = 1;

WQtExtractGeometryDlg::WQtExtractGeometryDlg(QWidget* pParent)

  : WQtDialog(pParent)
{
  setupUi(this);

  ExtractionMode->clear();
  ExtractionMode->addItem("Render Mesh");
  ExtractionMode->addItem("Collision Mesh");

  CoordinateSystem->clear();
  CoordinateSystem->addItem("Forward: +X, Right: +Y, Up: +Z (W)");
  CoordinateSystem->addItem("Forward: -Z, Right: +X, Up: +Y (OpenGL/Maya)");
  CoordinateSystem->addItem("Forward: +Z, Right: +X, Up: +Y (D3D)");

  UpdateUI();
}

void WQtExtractGeometryDlg::UpdateUI()
{
  DestinationFile->setText(s_sDestinationFile);
  ExtractOnlySelection->setChecked(s_bOnlySelection);
  ExtractionMode->setCurrentIndex(s_iExtractionMode);
  CoordinateSystem->setCurrentIndex(s_iCoordinateSystem);
}

void WQtExtractGeometryDlg::QueryUI()
{
  s_sDestinationFile = DestinationFile->text();
  s_bOnlySelection = ExtractOnlySelection->isChecked();
  s_iExtractionMode = ExtractionMode->currentIndex();
  s_iCoordinateSystem = CoordinateSystem->currentIndex();
}

void WQtExtractGeometryDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();

    if (!WPathUtils::IsAbsolutePath(s_sDestinationFile.toUtf8().data()))
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("Only absolute paths are allowed for the destination file.");
      return;
    }

    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}

void WQtExtractGeometryDlg::on_BrowseButton_clicked()
{
  QString allFilters = "OBJ (*.obj)";
  QString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Destination file"), s_sDestinationFile, allFilters,
    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  DestinationFile->setText(sFile);
}

WMat3 WQtExtractGeometryDlg::GetCoordinateSystemTransform()
{
  WMat3 m;
  m.SetIdentity();

  switch (s_iCoordinateSystem)
  {
    case 0:
      break;

    case 1:
      m.SetRow(2, WVec3(-1, 0, 0)); // forward
      m.SetRow(1, WVec3(0, 0, 1));  // up
      m.SetRow(0, WVec3(0, 1, 0));  // right
      break;

    case 2:
      m.SetRow(2, WVec3(1, 0, 0)); // forward
      m.SetRow(1, WVec3(0, 0, 1)); // up
      m.SetRow(0, WVec3(0, 1, 0)); // right
      break;
  }

  return m;
}
