#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Dialogs/CreateMeshLodsDlg.moc.h>
#include <QPushButton>

WInt32 WQtCreateMeshLodsDlg::s_iLodCount = 2;
bool WQtCreateMeshLodsDlg::s_bOpenAfterCreate = false;

WQtCreateMeshLodsDlg::WQtCreateMeshLodsDlg(const WMeshLodSource& source, QWidget* pParent)
  : WQtDialog(pParent)
  , m_pSource(&source)
{
  Setup();

  {
    // not editable: the prefab tool finds LODs by this exact name
    WStringBuilder sFolder = m_pSource->m_sLodFolder;
    WQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sFolder);
    LodFolder->setText(WMakeQString(sFolder));
  }

  UpdateInfo();
}

WQtCreateMeshLodsDlg::WQtCreateMeshLodsDlg(WUInt32 uiMeshCount, QWidget* pParent)
  : WQtDialog(pParent)
  , m_uiMeshCount(uiMeshCount)
{
  Setup();

  LodFolder->setText(QLatin1String("<next to each mesh asset>"));

  // Each mesh gets several LODs, so the document count grows faster than the mesh count. The box
  // stays available, it is only unticked by default for a large selection.
  OpenAfterCreate->setText(QString("Open all %1 LOD meshes after creating them").arg(uiMeshCount * s_iLodCount));
  OpenAfterCreate->setChecked(s_bOpenAfterCreate && uiMeshCount <= s_uiMaxAutoOpen);

  UpdateInfo();
}

void WQtCreateMeshLodsDlg::Setup()
{
  setupUi(this);

  LodCount->setMaximum((int)WMeshLodCreator::s_uiMaxLods);
  LodCount->setValue(s_iLodCount);
  OpenAfterCreate->setChecked(s_bOpenAfterCreate);
}

void WQtCreateMeshLodsDlg::UpdateInfo()
{
  const WUInt32 uiLodCount = (WUInt32)LodCount->value();
  const bool bOverwrite = OverwriteExisting->isChecked();

  {
    // how far each level goes depends on the mesh's own simplification, which isn't visible otherwise
    const WUInt8 uiBase = (m_pSource != nullptr) ? m_pSource->m_uiBaseSimplification : 0;

    // the percent signs are appended: a literal '%' in a format string is consumed as a format spec
    WStringBuilder sLadder;
    sLadder.SetFormat("LOD 0 (the mesh itself): {}", (WUInt32)uiBase);
    sLadder.Append("% simplified.");

    for (WUInt32 uiLod = 1; uiLod <= uiLodCount; ++uiLod)
    {
      sLadder.AppendFormat("\nLOD {}: {}", uiLod, (WUInt32)WMeshLodCreator::GetLodSimplification(uiBase, uiLod));
      sLadder.Append("% simplified.");
    }

    LadderInfo->setText(WMakeQString(sLadder));
  }

  WStringBuilder sWarning;

  // not every message blocks: that some LODs are kept still leaves something to create
  bool bCanCreate = true;

  if (m_pSource == nullptr)
  {
    // nothing mesh specific to check, the per mesh cases are handled while creating
  }
  else if (m_pSource->m_bIsPrimitive)
  {
    sWarning = "This mesh asset uses a procedural primitive, not a model file, so no LODs can be generated from it.";
    bCanCreate = false;
  }
  else if (m_pSource->m_sMeshFile.IsEmpty())
  {
    sWarning = "The source file of this mesh asset could not be read, so no LODs can be generated from it.";
    bCanCreate = false;
  }
  else
  {
    WHybridArray<WUInt32, 4> existing;
    for (WUInt32 uiLod = 1; uiLod <= uiLodCount; ++uiLod)
    {
      if (m_pSource->HasLod(uiLod))
      {
        existing.PushBack(uiLod);
      }
    }

    // with the overwrite box ticked, its own label already says what happens
    if (!existing.IsEmpty() && !bOverwrite)
    {
      WStringBuilder sList;
      for (WUInt32 uiLod : existing)
      {
        if (!sList.IsEmpty())
          sList.Append(", ");

        sList.AppendFormat("LOD-{}", uiLod);
      }

      // every requested level already there means there is nothing to do
      if (existing.GetCount() == uiLodCount)
      {
        sWarning = "All of the requested LODs already exist. Raise the count, or tick the box above to overwrite them.";
        bCanCreate = false;
      }
      else
      {
        sWarning.SetFormat("{} already exist and are kept.", sList);
      }
    }
  }

  WarningLabel->setText(WMakeQString(sWarning));

  if (QPushButton* pOk = ButtonBox->button(QDialogButtonBox::Ok))
  {
    pOk->setEnabled(bCanCreate);
  }
}

void WQtCreateMeshLodsDlg::on_LodCount_valueChanged(int value)
{
  UpdateInfo();
}

void WQtCreateMeshLodsDlg::on_OverwriteExisting_toggled(bool checked)
{
  UpdateInfo();
}

void WQtCreateMeshLodsDlg::on_ButtonBox_accepted()
{
  s_iLodCount = LodCount->value();

  // Not remembered for a large selection, where the box was unticked by us rather than by the user.
  if (m_uiMeshCount <= s_uiMaxAutoOpen)
  {
    s_bOpenAfterCreate = OpenAfterCreate->isChecked();
  }

  m_Options.m_uiLodCount = (WUInt32)s_iLodCount;
  m_Options.m_bOverwriteExisting = OverwriteExisting->isChecked();
  m_Options.m_bOpenAfterCreate = OpenAfterCreate->isChecked();

  accept();
}

void WQtCreateMeshLodsDlg::on_ButtonBox_rejected()
{
  reject();
}
