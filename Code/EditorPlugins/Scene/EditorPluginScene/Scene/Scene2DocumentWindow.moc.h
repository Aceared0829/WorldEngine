#pragma once

#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>

class WScene2Document;

class WQtScene2DocumentWindow : public WQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  WQtScene2DocumentWindow(WScene2Document* pDocument);
  ~WQtScene2DocumentWindow();

  virtual bool InternalCanCloseWindow() override;

  WStatus SaveAllLayers();
};
