#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class WQtSearchWidget;
class WQtDocumentTreeView;
class WSceneDocument;
class WScene2Document;
class QStackedWidget;
struct WScene2LayerEvent;

class WQtScenegraphPanel : public WQtDocumentPanel
{
  Q_OBJECT

public:
  WQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, WSceneDocument* pDocument);
  WQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, WScene2Document* pDocument);
  ~WQtScenegraphPanel();

private:
  void LayerEventHandler(const WScene2LayerEvent& e);
  void LayerLoaded(const WUuid& layerGuid);
  void LayerUnloaded(const WUuid& layerGuid);
  void ActiveLayerChanged(const WUuid& layerGuid);

private:
  WSceneDocument* m_pSceneDocument;
  QStackedWidget* m_pStack = nullptr;
  WQtGameObjectWidget* m_pMainGameObjectWidget = nullptr;
  WEvent<const WScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  WMap<WUuid, WQtGameObjectWidget*> m_LayerWidgets;
};
