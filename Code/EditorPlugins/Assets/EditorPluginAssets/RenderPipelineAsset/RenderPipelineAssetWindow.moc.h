#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class WQtVisualGraphScene;
class WQtVisualGraphView;

class WQtRenderPipelineAssetDocumentWindow : public WQtDocumentWindow
{
  Q_OBJECT

public:
  WQtRenderPipelineAssetDocumentWindow(WDocument* pDocument);
  ~WQtRenderPipelineAssetDocumentWindow();

private Q_SLOTS:

private:
  // Both are owned by Qt: the scene through its parent, the view through the panel it is set on.
  WQtVisualGraphScene* m_pScene = nullptr;
  WQtVisualGraphView* m_pView = nullptr;
};
