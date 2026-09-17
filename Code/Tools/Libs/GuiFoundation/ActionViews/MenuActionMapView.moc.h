#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenu>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class WActionMap;
class QAction;
class WQtProxy;


class W_GUIFOUNDATION_DLL WQtMenuActionMapView : public QMenu
{
  Q_OBJECT
  W_DISALLOW_COPY_AND_ASSIGN(WQtMenuActionMapView);

public:
  explicit WQtMenuActionMapView(QWidget* pParent);
  ~WQtMenuActionMapView();

  void SetActionContext(const WActionContext& context);

  static void AddDocumentObjectToMenu(WHashTable<WUuid, QSharedPointer<WQtProxy>>& ref_proxies, WActionContext& ref_context, WActionMap* pActionMap,
    QMenu* pCurrentRoot, const WActionMap::TreeNode* pObject);

private:
  void ClearView();
  void CreateView();

private:
  WHashTable<WUuid, QSharedPointer<WQtProxy>> m_Proxies;

  WActionContext m_Context;
  WActionMap* m_pActionMap;
};
