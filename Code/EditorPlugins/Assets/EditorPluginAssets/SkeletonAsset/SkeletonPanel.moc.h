#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class WSkeletonAssetDocument;
class QTreeView;
class WQtDocumentTreeView;
class WQtSearchWidget;

class WQtSkeletonPanel : public WQtDocumentPanel
{
  Q_OBJECT

public:
  WQtSkeletonPanel(ads::CDockManager* pDockManager, QWidget* pParent, WSkeletonAssetDocument* pDocument);
  ~WQtSkeletonPanel();

private:
  WSkeletonAssetDocument* m_pSkeletonDocument = nullptr;
  QWidget* m_pMainWidget = nullptr;
  WQtDocumentTreeView* m_pTreeWidget = nullptr;
  WQtSearchWidget* m_pFilterWidget = nullptr;
};
