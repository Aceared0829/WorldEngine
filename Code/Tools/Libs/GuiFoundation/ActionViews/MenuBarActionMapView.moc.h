#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenuBar>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class WActionMap;
class QAction;
class WQtProxy;

class W_GUIFOUNDATION_DLL WQtMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  W_DISALLOW_COPY_AND_ASSIGN(WQtMenuBarActionMapView);

public:
  explicit WQtMenuBarActionMapView(QWidget* pParent);
  ~WQtMenuBarActionMapView();

  void SetActionContext(const WActionContext& context);

private:
  void TreeEventHandler(const WDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();

private:
  WHashTable<WUuid, QSharedPointer<WQtProxy>> m_Proxies;

  WActionContext m_Context;
  WActionMap* m_pActionMap;
};
