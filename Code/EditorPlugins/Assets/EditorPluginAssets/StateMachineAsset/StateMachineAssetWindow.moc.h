#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WQtStateMachineAssetScene;
class WQtVisualGraphView;

class WQtStateMachineAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtStateMachineAssetDocumentWindow(WDocument* pDocument);
  ~WQtStateMachineAssetDocumentWindow();

private Q_SLOTS:

private:
  WQtStateMachineAssetScene* m_pScene;
  WQtVisualGraphView* m_pView;
};
