#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetImportDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WQtAssetImportDlg : public WQtDialog, public Ui_AssetImportDlg
{
  Q_OBJECT

public:
  WQtAssetImportDlg(QWidget* pParent, WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions>& ref_allImports);
  ~WQtAssetImportDlg();

private Q_SLOTS:
  void SelectedOptionChanged(int index);
  void on_ButtonImport_clicked();

private:
  void InitRow(WUInt32 uiRow);

  WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions>& m_AllImports;
};
