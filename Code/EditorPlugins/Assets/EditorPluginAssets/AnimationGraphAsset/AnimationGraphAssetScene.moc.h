#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

class WQtVisualGraphScene;
class WQtVisualGraphView;

/// Qt scene for animation graph asset editing.
///
/// Manages the visual scene for editing animation graph assets in the editor.
class WQtAnimationGraphAssetScene : public WQtVisualGraphScene
{
  Q_OBJECT

public:
  WQtAnimationGraphAssetScene(QObject* pParent = nullptr);
  ~WQtAnimationGraphAssetScene();
};
