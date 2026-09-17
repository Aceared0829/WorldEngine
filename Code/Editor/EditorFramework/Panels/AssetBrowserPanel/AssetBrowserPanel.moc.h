#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

class QStatusBar;
class QLabel;
struct WToolsProjectEvent;

/// The application wide panel that shows and asset browser.
class W_EDITORFRAMEWORK_DLL WQtAssetBrowserPanel : public WQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

  W_DECLARE_SINGLETON(WQtAssetBrowserPanel);

public:
  WQtAssetBrowserPanel(ads::CDockManager* pDockManager);
  ~WQtAssetBrowserPanel();

  const WUuid& GetLastSelectedAsset() const { return m_LastSelected; }

private Q_SLOTS:
  void SlotAssetChosen(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void SlotAssetSelected(WUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, WUInt8 uiAssetBrowserItemFlags);
  void SlotAssetCleared();

private:
  void AssetCuratorEvents(const WAssetCuratorEvent& e);
  void ProjectEvents(const WToolsProjectEvent& e);

  WUuid m_LastSelected;
};
