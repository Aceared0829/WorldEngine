#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class WScene2Document;
class WQtLayerDelegate;

class WQtLayerPanel : public WQtDocumentPanel
{
  Q_OBJECT

public:
  WQtLayerPanel(ads::CDockManager* pDockManager, QWidget* pParent, WScene2Document* pDocument);
  ~WQtLayerPanel();

private Q_SLOTS:
  void OnRequestContextMenu(QPoint pos);

private:
  WQtLayerDelegate* m_pDelegate = nullptr;
  WScene2Document* m_pSceneDocument = nullptr;
  WQtDocumentTreeView* m_pTreeWidget = nullptr;
  WString m_sContextMenuMapping;
};
