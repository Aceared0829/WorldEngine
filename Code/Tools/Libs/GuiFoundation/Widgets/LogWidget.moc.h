#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <GuiFoundation/ui_LogWidget.h>
#include <QTextDocument>
#include <QWidget>

class WQtLogModel;
class WQtSearchWidget;

/// Renders log entries that contain embedded [[text|target]] links and handles navigation on double-click or middle-click.
class WQtLogWidgetItemDelegate : public WQtItemDelegate
{
  Q_OBJECT
public:
  WQtLogWidgetItemDelegate(QObject* pParent);
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool mouseHoverEvent(QHoverEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseDoubleClickEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
  QPersistentModelIndex m_HoveredIndex;
  mutable QTextDocument m_Doc;
};

/// The application wide panel that shows the engine log output and the editor log output
class W_GUIFOUNDATION_DLL WQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  WQtLogWidget(QWidget* pParent);
  ~WQtLogWidget();

  void ShowControls(bool bShow);

  WQtLogModel* GetLog();
  WQtSearchWidget* GetSearchWidget();
  void SetLogLevel(WLogMsgType::Enum logLevel);
  WLogMsgType::Enum GetLogLevel() const;

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  using LogItemContextActionCallback = WDelegate<void(const WStringView& sLogText)>;
  static bool AddLogItemContextActionCallback(const WStringView& sName, const LogItemContextActionCallback& logCallback);
  static bool RemoveLogItemContextActionCallback(const WStringView& sName);

private Q_SLOTS:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);
  void OnItemDoubleClicked(QModelIndex idx);

private:
  WQtLogModel* m_pLog;
  void ScrollToBottomIfAtEnd(int iFirstNewRow);

  WQtLogWidgetItemDelegate* m_pDelegate = nullptr;

  /// List of callbacks invoked when the user double clicks a log message
  static WMap<WString, LogItemContextActionCallback> s_LogCallbacks;
};
