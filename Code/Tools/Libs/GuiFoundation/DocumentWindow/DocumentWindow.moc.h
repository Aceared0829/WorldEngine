#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

#include <QMainWindow>
#include <ads/DockManager.h>

class WQtContainerWindow;
class WDocument;
class WQtDocumentWindow;
class QLabel;
class QToolButton;

struct WQtDocumentWindowEvent
{
  enum Type
  {
    WindowClosing,           ///< Sent shortly before the window is being deleted
    WindowClosed,            ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
    WindowDecorationChanged, ///< Window title or icon has changed
    BeforeRedraw,            ///< Sent shortly before the content of the window is being redrawn
  };

  Type m_Type;
  WQtDocumentWindow* m_pWindow = nullptr;
};

/// Base class for all document windows. Handles the most basic document window management.
class W_GUIFOUNDATION_DLL WQtDocumentWindow : public QMainWindow
{
  Q_OBJECT

public:
  static WEvent<const WQtDocumentWindowEvent&> s_Events;

public:
  WQtDocumentWindow(WDocument* pDocument);
  WQtDocumentWindow(const char* szUniqueName);
  virtual ~WQtDocumentWindow();

  ads::CDockManager* m_pDockManager = nullptr;

  void EnsureVisible();

  virtual WString GetWindowIcon() const;
  virtual WString GetDisplayName() const { return GetUniqueName(); }
  virtual WString GetDisplayNameShort() const;

  const char* GetUniqueName() const { return m_sUniqueName; }

  WDocument* GetDocument() const { return m_pDocument; }

  WStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFramerate(WInt16 iTargetFPS);

  void TriggerRedraw();

  virtual void RequestWindowTabContextMenu(const QPoint& globalPos);

  static const WDynamicArray<WQtDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  /// Returns the document window for the given document, if there is any. nullptr otherwise.
  static WQtDocumentWindow* FindWindowByDocument(const WDocument* pDocument);
  WQtContainerWindow* GetContainerWindow() const;

  /// Shows the given message for the given duration in the statusbar, then shows the permanent message again.
  void ShowTemporaryStatusBarMsg(const WFormatString& text, WTime duration = WTime::MakeFromSeconds(5));

  /// Sets which text to show permanently in the statusbar. Set an empty string to clear the message.
  void SetPermanentStatusBarMsg(const WFormatString& text);

  /// For unit tests to take a screenshot of the window (may include multiple views) to do image comparisons.
  virtual void CreateImageCapture(const char* szOutputPath);

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void hideEvent(QHideEvent* event) override;
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  void FinishWindowCreation();

  /// Called after a saved dock layout has been restored. Override to re-apply any panel visibility
  /// that must not be overridden by old layout files (e.g. panels that are conditionally shown/hidden).
  /// The base implementation does nothing.
  virtual void OnAfterDocumentLayoutRestored() {}

private Q_SLOTS:
  void SlotRedraw();
  void SlotQueuedDelete();
  void OnPermanentGlobalStatusClicked(bool);
  void OnStatusBarMessageChanged(const QString& sNewText);
  void SlotRestoreDocumentLayout();
  void SlotCaptureInitialLayoutState();

private:
  void ShutdownDocumentWindow();

private:
  friend class WQtContainerWindow;

  void SetVisibleInContainer(bool bVisible);
  bool m_bIsVisibleInContainer = false;
  bool m_bRedrawIsTriggered = false;
  bool m_bIsDrawingATM = false;
  bool m_bTriggerRedrawQueued = false;
  WInt16 m_iTargetFramerate = 0;
  WDocument* m_pDocument = nullptr;
  WQtContainerWindow* m_pContainerWindow = nullptr;
  QLabel* m_pPermanentDocumentStatusText = nullptr;
  QToolButton* m_pPermanentGlobalStatusButton = nullptr;
  QByteArray m_InitialDocumentLayoutState;

private:
  void Constructor();
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);
  void DocumentEventHandler(const WDocumentEvent& e);
  void UIServicesEventHandler(const WQtUiServices::Event& e);
  void UIServicesTickEventHandler(const WQtUiServices::TickEvent& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) {}
  virtual void InternalRedraw() {}

  WString m_sUniqueName;

  static WDynamicArray<WQtDocumentWindow*> s_AllDocumentWindows;
};
