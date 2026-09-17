#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WSelectionContext;

class WQtSkeletonAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtSkeletonAssetDocumentWindow(WSkeletonAssetDocument* pDocument);
  ~WQtSkeletonAssetDocumentWindow();

  WSkeletonAssetDocument* GetSkeletonDocument();

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const WEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(WInt32 iPurpose = 0);
  void SelectionEventHandler(const WSelectionManagerEvent& e);
  void SkeletonAssetEventHandler(const WSkeletonAssetEvent& e);

  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void CommandEventHandler(const WCommandHistoryEvent&);

  void SendLiveResourcePreview();
  void RestoreResource();

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget = nullptr;
};
