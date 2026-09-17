#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class WQtSearchWidget;
class WGameObjectDocument;
struct WGameObjectEvent;
class WQtGameObjectDelegate;

class W_EDITORFRAMEWORK_DLL WQtGameObjectWidget : public QWidget
{
  Q_OBJECT

public:
  WQtGameObjectWidget(QWidget* pParent, WGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<WQtDocumentTreeModel> pCustomModel, WSelectionManager* pSelection = nullptr);
  ~WQtGameObjectWidget();

  WQtSearchWidget& GetFilterWidget() { return *m_pFilterWidget; }

private Q_SLOTS:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const WGameObjectEvent& e);

protected:
  WQtGameObjectDelegate* m_pDelegate = nullptr;
  WGameObjectDocument* m_pDocument;
  WQtDocumentTreeView* m_pTreeWidget;
  WQtSearchWidget* m_pFilterWidget;
  WString m_sContextMenuMapping;
};

class W_EDITORFRAMEWORK_DLL WQtGameObjectPanel : public WQtDocumentPanel
{
  Q_OBJECT

public:
  WQtGameObjectPanel(ads::CDockManager* pDockManager, QWidget* pParent, WGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<WQtDocumentTreeModel> pCustomModel);
  ~WQtGameObjectPanel();


protected:
  WQtGameObjectWidget* m_pMainWidget = nullptr;
};
