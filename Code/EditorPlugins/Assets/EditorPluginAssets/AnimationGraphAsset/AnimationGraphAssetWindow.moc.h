#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtAnimationGraphAssetScene;
class WQtVisualGraphView;

class WQtAnimationGraphAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtAnimationGraphAssetDocumentWindow(WDocument* pDocument);
  ~WQtAnimationGraphAssetDocumentWindow();

private Q_SLOTS:

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  WQtAnimationGraphAssetScene* m_pScene;
  WQtVisualGraphView* m_pView;
};
