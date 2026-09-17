#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetCheckPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class QTreeWidgetItem;
namespace ads
{
  class CDockManager;
}

/// Application wide panel that runs asset check rules over a selection of assets and lists the reported issues.
class W_EDITORFRAMEWORK_DLL WQtAssetCheckPanel : public WQtApplicationPanel, public Ui_AssetCheckPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtAssetCheckPanel);

public:
  WQtAssetCheckPanel(ads::CDockManager* pDockManager);
  ~WQtAssetCheckPanel();

  void FillRuleList();

protected:
  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
  void RunButtonClicked();
  void ResultTreeItemDoubleClicked(QTreeWidgetItem* pItem, int iColumn);

  void UpdateAssetTypeCombo();
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);

  WDynamicArray<WAssetCheckRule*> m_Rules;
};
