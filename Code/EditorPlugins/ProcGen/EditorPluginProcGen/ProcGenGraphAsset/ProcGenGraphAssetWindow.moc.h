#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WProcGenGraphAssetDocument;

class WQtVisualGraphScene;
class WQtVisualGraphView;
struct WCommandHistoryEvent;

class WProcGenGraphAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WProcGenGraphAssetDocumentWindow(WProcGenGraphAssetDocument* pDocument);
  ~WProcGenGraphAssetDocumentWindow();

  WProcGenGraphAssetDocument* GetProcGenGraphDocument();

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void TransactionEventHandler(const WCommandHistoryEvent& e);

  void SelectionEventHandler(const WSelectionManagerEvent& e);

  WQtVisualGraphScene* m_pScene;
  WQtVisualGraphView* m_pView;
};
