#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtEngineViewWidget;

class WQtRmlUiAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtRmlUiAssetDocumentWindow(WAssetDocument* pDocument);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();

  WEngineViewConfig m_ViewConfig;
  WQtEngineViewWidget* m_pViewWidget;
  WRmlUiAssetDocument* m_pAssetDoc;
};
