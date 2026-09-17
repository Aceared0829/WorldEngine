#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtVisualScriptNodeScene;
class WQtVisualGraphView;

class WQtVisualScriptWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtVisualScriptWindow(WDocument* pDocument);
  ~WQtVisualScriptWindow();

private Q_SLOTS:

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  WQtVisualScriptNodeScene* m_pScene;
  WQtVisualGraphView* m_pView;
};
