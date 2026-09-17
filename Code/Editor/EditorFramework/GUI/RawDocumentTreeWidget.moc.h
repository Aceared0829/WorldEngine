#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <memory>

class WQtTreeSearchFilterModel;
class WSelectionManager;
class WGameObjectFilter;

class W_EDITORFRAMEWORK_DLL WQtDocumentTreeView : public WQtItemView<QTreeView>
{
  Q_OBJECT

public:
  WQtDocumentTreeView(QWidget* pParent);
  WQtDocumentTreeView(QWidget* pParent, WDocument* pDocument, std::unique_ptr<WQtDocumentTreeModel> pCustomModel, WSelectionManager* pSelection = nullptr);
  ~WQtDocumentTreeView();

  void Initialize(WDocument* pDocument, std::unique_ptr<WQtDocumentTreeModel> pCustomModel, WSelectionManager* pSelection = nullptr);

  void EnsureLastSelectedItemVisible();

  void SetAllowDragDrop(bool bAllow);
  void SetAllowDeleteObjects(bool bAllow);

  WQtTreeSearchFilterModel* GetProxyFilterModel() const { return m_pFilterModel.get(); }

protected:
  virtual bool event(QEvent* pEvent) override;

private Q_SLOTS:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

private:
  std::unique_ptr<WQtDocumentTreeModel> m_pModel;
  std::unique_ptr<WQtTreeSearchFilterModel> m_pFilterModel;
  std::unique_ptr<WGameObjectFilter> m_pGameObjectFilter;
  WSelectionManager* m_pSelectionManager = nullptr;
  WDocument* m_pDocument = nullptr;
  bool m_bBlockSelectionSignal = false;
  bool m_bAllowDeleteObjects = false;
};
