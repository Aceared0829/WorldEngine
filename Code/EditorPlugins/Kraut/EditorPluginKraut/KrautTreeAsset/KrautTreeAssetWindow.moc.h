#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WQtPropertyGridWidget;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

/// Editor document window for a Kraut tree asset.
///
/// The window listens to property changes and sends redraw messages to the engine process
/// to keep the preview in sync with the current asset state.
class WQtKrautTreeAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtKrautTreeAssetDocumentWindow(WAssetDocument* pDocument);
  ~WQtKrautTreeAssetDocumentWindow();

  WKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<WKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);

private Q_SLOTS:
  void onBranchTypeSelected(int index);
  void onLodSelected(int index);
  void onAutoLodToggled(bool bChecked);
  void onAddLod();
  void onDeleteLod();
  void onImportKrautFile();
  void onCopyBranchType();
  void onPasteBranchType();
  void onCopyLod();
  void onPasteLod();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(WInt32 iPurpose = 0);
  void UpdatePreview();
  void RestoreResource();
  void ImportKrautFile();
  void RebuildBranchTypeCombo();
  void RebuildLodCombo();
  void ApplyLodPreset(int iPresetIndex);
  WDocumentObject* GetCurrentBranchTypeObject() const;
  WDocumentObject* GetCurrentLodObject() const;
  void CopyObjectToClipboard(const WDocumentObject* pObject, const char* szMimeType);
  void PasteObjectFromClipboard(WDocumentObject* pObject, const char* szMimeType, const char* szTransactionName);

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  WKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  WQtPropertyGridWidget* m_pBranchProps = nullptr;
  WQtPropertyGridWidget* m_pLodProps = nullptr;
  QComboBox* m_pBranchTypeCombo = nullptr;
  QComboBox* m_pLodCombo = nullptr;
  QPushButton* m_pAddLodButton = nullptr;
  QPushButton* m_pDeleteLodButton = nullptr;
  QCheckBox* m_pAutoLodCheckbox = nullptr;
  int m_iCurrentLodIndex = 0; ///< Index of the LOD shown in the preview; -1 = full detail (no reduction), 0-4 = forced LOD index.

  QLabel* m_pStatsBones = nullptr;
  QLabel* m_pStatsTotal = nullptr;
  QLabel* m_pStatsBranch = nullptr;
  QLabel* m_pStatsFrond = nullptr;
  QLabel* m_pStatsLeaves = nullptr;
};
