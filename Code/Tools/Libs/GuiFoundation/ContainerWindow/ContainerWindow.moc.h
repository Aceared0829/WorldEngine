#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMainWindow>
#include <QSet>
#include <ToolsFoundation/Project/ToolsProject.h>

class WDocumentManager;
class WDocument;
class WQtApplicationPanel;
struct WDocumentTypeDescriptor;
class QLabel;

namespace ads
{
  class CDockManager;
  class CFloatingDockContainer;
  class CDockWidget;
} // namespace ads

/// Container window that hosts documents and applications panels.
class W_GUIFOUNDATION_DLL WQtContainerWindow : public QMainWindow
{
  Q_OBJECT

public:
  /// Constructor.
  WQtContainerWindow();
  ~WQtContainerWindow();

  static WQtContainerWindow* GetContainerWindow() { return s_pContainerWindow; }

  void AddDocumentWindow(WQtDocumentWindow* pDocWindow);
  void DocumentWindowRenamed(WQtDocumentWindow* pDocWindow);
  void AddApplicationPanel(WQtApplicationPanel* pPanel);

  ads::CDockManager* GetDockManager() { return m_pDockManager; }

  static WResult EnsureVisibleAnyContainer(WDocument* pDocument);

  void GetDocumentWindows(WHybridArray<WQtDocumentWindow*, 16>& ref_windows);

  struct DocumentWindowState
  {
    bool m_bFloating = false;
  };

  /// Saves the current state (floating/docked) of all document windows.
  /// Call before restoring a layout.
  void SaveDocumentWindowStates(WMap<ads::CDockWidget*, DocumentWindowState>& out_states);

  /// Restores document windows to their previous states after a layout change.
  /// Call after restoring a layout.
  void RestoreDocumentWindowStates(const WMap<ads::CDockWidget*, DocumentWindowState>& states);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

private:
  friend class WQtDocumentWindow;
  friend class WQtApplicationPanel;

  WResult EnsureVisible(WQtDocumentWindow* pDocWindow);
  WResult EnsureVisible(WDocument* pDocument);
  WResult EnsureVisible(WQtApplicationPanel* pPanel);

private Q_SLOTS:
  void SlotDocumentTabCloseRequested();
  void SlotTabsContextMenuRequested(const QPoint& pos);
  void SlotUpdateWindowDecoration(void* pDocWindow);
  void SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget);
  void SlotDockWidgetFloatingChanged(bool bFloating);

private:
  void UpdateWindowTitle();

  void RemoveDocumentWindow(WQtDocumentWindow* pDocWindow);
  void RemoveApplicationPanel(WQtApplicationPanel* pPanel);

  void UpdateWindowDecoration(WQtDocumentWindow* pDocWindow);

  void DocumentWindowEventHandler(const WQtDocumentWindowEvent& e);
  void ProjectEventHandler(const WToolsProjectEvent& e);
  void UIServicesEventHandler(const WQtUiServices::Event& e);

  virtual void closeEvent(QCloseEvent* e) override;

private:
  ads::CDockManager* m_pDockManager = nullptr;
  QLabel* m_pStatusBarLabel;
  WDynamicArray<WQtDocumentWindow*> m_DocumentWindows;
  WDynamicArray<ads::CDockWidget*> m_DocumentDocks;

  WDynamicArray<WQtApplicationPanel*> m_ApplicationPanels;
  QSet<QString> m_DockNames;

  static WQtContainerWindow* s_pContainerWindow;
  static bool s_bForceClose;
};
