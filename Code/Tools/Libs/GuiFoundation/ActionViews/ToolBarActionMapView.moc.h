#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QSharedPointer>
#include <QToolBar>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class WActionMap;
class QAction;
class WQtProxy;
class QMenu;

class W_GUIFOUNDATION_DLL WQtToolBarActionMapView : public QToolBar
{
  Q_OBJECT
  W_DISALLOW_COPY_AND_ASSIGN(WQtToolBarActionMapView);

public:
  explicit WQtToolBarActionMapView(QString sTitle, QWidget* pParent);
  ~WQtToolBarActionMapView();

  void SetActionContext(const WActionContext& context);

  virtual void setVisible(bool bVisible) override;

private:
  void TreeEventHandler(const WDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(const WActionMap::TreeNode* pRoot);

private:
  WHashTable<WUuid, QSharedPointer<WQtProxy>> m_Proxies;

  WActionContext m_Context;
  WActionMap* m_pActionMap;
};
