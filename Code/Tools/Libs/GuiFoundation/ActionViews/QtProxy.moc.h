#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QWidget>
#include <QWidgetAction>

class QAction;
class QMenu;
class QLabel;
class QSlider;
class WAction;

/// Glue class that maps WActions to QActions. QActions are only created if the WAction is actually mapped somewhere. Document and Global actions are manually executed and don't solely rely on Qt's ShortcutContext setting to prevent ambiguous action shortcuts.
class W_GUIFOUNDATION_DLL WQtProxy : public QObject
{
  Q_OBJECT

public:
  WQtProxy();
  virtual ~WQtProxy();

  virtual void Update() = 0;

  virtual void SetAction(WAction* pAction);
  WAction* GetAction() { return m_pAction; }

  /// Converts the QKeyEvent into a shortcut and tries to find a matching action in the document and global action list.
  ///
  /// Document actions are not mapped as ShortcutContext::WindowShortcut because docking allows for multiple documents to be mapped into the same window. Instead, ShortcutContext::WidgetWithChildrenShortcut is used to prevent ambiguous action shortcuts and the actions are executed manually via filtering QEvent::ShortcutOverride at the dock widget level.
  /// The function always has to be called two times:
  /// A: QEvent::ShortcutOverride: Only check with bTestOnly = true that we want to override the shortcut. This will instruct Qt to send the event as a regular key press event to the widget that accepted the override.
  /// B: QEvent::keyPressEvent: Execute the actual action with bTestOnly = false;
  ///
  /// \param pDocument The document for which matching actions should be searched for. If null, only global actions are searched.
  /// \param pEvent The key event that should be converted into a shortcut.
  /// \param bTestOnly Accept the event and return true but don't execute the action. Use this inside QEvent::ShortcutOverride.
  /// \return Whether the key event was consumed and an action executed.
  static bool TriggerDocumentAction(WDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly);

  static WRttiMappedObjectFactory<WQtProxy>& GetFactory();
  static QSharedPointer<WQtProxy> GetProxy(WActionContext& ref_context, WActionDescriptorHandle hAction);

protected:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, QtProxies);
  static WRttiMappedObjectFactory<WQtProxy> s_Factory;
  static WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>> s_GlobalActions;
  static WMap<const WDocument*, WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>> s_DocumentActions;
  static WMap<QWidget*, WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>> s_WindowActions;
  static QObject* s_pSignalProxy;

protected:
  WAction* m_pAction;
};

class W_GUIFOUNDATION_DLL WQtActionProxy : public WQtProxy
{
  Q_OBJECT

public:
  virtual QAction* GetQAction() = 0;
};

class W_GUIFOUNDATION_DLL WQtCategoryProxy : public WQtProxy
{
  Q_OBJECT
public:
  virtual void Update() override {}
};

class W_GUIFOUNDATION_DLL WQtMenuProxy : public WQtProxy
{
  Q_OBJECT

public:
  WQtMenuProxy();
  ~WQtMenuProxy();

  virtual void Update() override;
  virtual void SetAction(WAction* pAction) override;

  virtual QMenu* GetQMenu();

private:
  void StatusUpdateEventHandler(WAction* pAction);

protected:
  QMenu* m_pMenu;
};

class W_GUIFOUNDATION_DLL WQtButtonProxy : public WQtActionProxy
{
  Q_OBJECT

public:
  WQtButtonProxy();
  ~WQtButtonProxy();

  virtual void Update() override;
  virtual void SetAction(WAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(WAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class W_GUIFOUNDATION_DLL WQtDynamicMenuProxy : public WQtMenuProxy
{
  Q_OBJECT

public:
  virtual void SetAction(WAction* pAction) override;

private Q_SLOTS:
  void SlotMenuAboutToShow();
  void SlotMenuEntryTriggered();

private:
  WHybridArray<WDynamicMenuAction::Item, 16> m_Entries;
};

class W_GUIFOUNDATION_DLL WQtDynamicActionAndMenuProxy : public WQtDynamicMenuProxy
{
  Q_OBJECT

public:
  WQtDynamicActionAndMenuProxy();
  ~WQtDynamicActionAndMenuProxy();

  virtual void Update() override;
  virtual void SetAction(WAction* pAction) override;
  virtual QAction* GetQAction();

private Q_SLOTS:
  void OnTriggered();

private:
  QPointer<QAction> m_pQtAction;
};


class W_GUIFOUNDATION_DLL WQtLabeledSlider : public QWidget
{
  Q_OBJECT

public:
  WQtLabeledSlider(QWidget* pParent);

  QLabel* m_pLabel;
  QSlider* m_pSlider;
};


class W_GUIFOUNDATION_DLL WQtSliderWidgetAction : public QWidgetAction
{
  Q_OBJECT

public:
  WQtSliderWidgetAction(QWidget* pParent);
  void setMinimum(int value);
  void setMaximum(int value);
  void setValue(int value);

Q_SIGNALS:
  void valueChanged(int value);

private Q_SLOTS:
  void OnValueChanged(int value);

protected:
  virtual QWidget* createWidget(QWidget* parent) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  WInt32 m_iMinimum;
  WInt32 m_iMaximum;
  WInt32 m_iValue;
};

class W_GUIFOUNDATION_DLL WQtSliderProxy : public WQtActionProxy
{
  Q_OBJECT

public:
  WQtSliderProxy();
  ~WQtSliderProxy();

  virtual void Update() override;
  virtual void SetAction(WAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnValueChanged(int value);

private:
  void StatusUpdateEventHandler(WAction* pAction);

private:
  QPointer<WQtSliderWidgetAction> m_pQtAction;
};
