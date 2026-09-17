#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtOrbitCamViewWidget;
class WTextureCubeAssetDocument;

class WQtTextureCubeAssetDocumentWindow : public WQtEngineDocumentWindow
{
  Q_OBJECT

public:
  WQtTextureCubeAssetDocumentWindow(WTextureCubeAssetDocument* pDocument);

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  WEngineViewConfig m_ViewConfig;
  WQtOrbitCamViewWidget* m_pViewWidget;
};
