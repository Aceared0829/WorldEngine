#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QAction>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

WRttiMappedObjectFactory<WQtProxy> WQtProxy::s_Factory;
WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>> WQtProxy::s_GlobalActions;
WMap<const WDocument*, WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>> WQtProxy::s_DocumentActions;
WMap<QWidget*, WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>> WQtProxy::s_WindowActions;
QObject* WQtProxy::s_pSignalProxy = nullptr;

static WQtProxy* QtMenuProxyCreator(const WRTTI* pRtti)
{
  return new (WQtMenuProxy);
}

static WQtProxy* QtCategoryProxyCreator(const WRTTI* pRtti)
{
  return new (WQtCategoryProxy);
}

static WQtProxy* QtButtonProxyCreator(const WRTTI* pRtti)
{
  return new (WQtButtonProxy);
}

static WQtProxy* QtDynamicMenuProxyCreator(const WRTTI* pRtti)
{
  return new (WQtDynamicMenuProxy);
}

static WQtProxy* QtDynamicActionAndMenuProxyCreator(const WRTTI* pRtti)
{
  return new (WQtDynamicActionAndMenuProxy);
}

static WQtProxy* QtSliderProxyCreator(const WRTTI* pRtti)
{
  return new (WQtSliderProxy);
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtProxies)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WMenuAction>(), QtMenuProxyCreator);
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WCategoryAction>(), QtCategoryProxyCreator);
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WDynamicMenuAction>(), QtDynamicMenuProxyCreator);
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WDynamicActionAndMenuAction>(), QtDynamicActionAndMenuProxyCreator);
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WButtonAction>(), QtButtonProxyCreator);
    WQtProxy::GetFactory().RegisterCreator(WGetStaticRTTI<WSliderAction>(), QtSliderProxyCreator);
    WQtProxy::s_pSignalProxy = new QObject;
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WMenuAction>());
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WCategoryAction>());
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WDynamicMenuAction>());
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WDynamicActionAndMenuAction>());
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WButtonAction>());
    WQtProxy::GetFactory().UnregisterCreator(WGetStaticRTTI<WSliderAction>());
    WQtProxy::s_GlobalActions.Clear();
    WQtProxy::s_DocumentActions.Clear();
    WQtProxy::s_WindowActions.Clear();
    delete WQtProxy::s_pSignalProxy;
    WQtProxy::s_pSignalProxy = nullptr;
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool WQtProxy::TriggerDocumentAction(WDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly)
{
  auto CheckActions = [&](QKeyEvent* pEvent, WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>& ref_actions) -> bool
  {
    for (auto weakActionProxy : ref_actions)
    {
      if (auto pProxy = weakActionProxy.Value().toStrongRef())
      {
        QAction* pQAction = nullptr;
        if (auto pActionProxy = qobject_cast<WQtActionProxy*>(pProxy))
        {
          pQAction = pActionProxy->GetQAction();
        }
        else if (auto pActionProxy2 = qobject_cast<WQtDynamicActionAndMenuProxy*>(pProxy))
        {
          pQAction = pActionProxy2->GetQAction();
        }

        if (pQAction)
        {
          QKeySequence ks = pQAction->shortcut();
          if (pQAction->isEnabled() && QKeySequence(pEvent->key() | pEvent->modifiers()) == ks)
          {
            if (!bTestOnly)
            {
              pQAction->trigger();
            }
            pEvent->accept();
            return true;
          }
        }
      }
    }
    return false;
  };

  if (pDocument)
  {
    WMap<WActionDescriptorHandle, QWeakPointer<WQtProxy>>& actions = s_DocumentActions[pDocument];
    if (CheckActions(pEvent, actions))
      return true;
  }
  return CheckActions(pEvent, s_GlobalActions);
}

WRttiMappedObjectFactory<WQtProxy>& WQtProxy::GetFactory()
{
  return s_Factory;
}
QSharedPointer<WQtProxy> WQtProxy::GetProxy(WActionContext& ref_context, WActionDescriptorHandle hDesc)
{
  QSharedPointer<WQtProxy> pProxy;
  const WActionDescriptor* pDesc = hDesc.GetDescriptor();
  if (pDesc->m_Type != WActionType::Action && pDesc->m_Type != WActionType::ActionAndMenu)
  {
    auto pAction = pDesc->CreateAction(ref_context);
    pProxy = QSharedPointer<WQtProxy>(WQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
    W_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
    pProxy->SetAction(pAction);
    W_ASSERT_DEV(pProxy->GetAction()->GetContext().m_pDocument == ref_context.m_pDocument, "invalid document pointer");
    return pProxy;
  }

  // WActionType::Action will be cached to ensure only one QAction exist in its scope to prevent shortcut collisions.
  switch (pDesc->m_Scope)
  {
    case WActionScope::Global:
    {
      QWeakPointer<WQtProxy> pTemp = s_GlobalActions[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy = QSharedPointer<WQtProxy>(WQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        W_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_GlobalActions[hDesc] = pProxy.toWeakRef();
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case WActionScope::Document:
    {
      const WDocument* pDocument = ref_context.m_pDocument; // may be null

      QWeakPointer<WQtProxy> pTemp = s_DocumentActions[pDocument][hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy = QSharedPointer<WQtProxy>(WQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        W_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_DocumentActions[pDocument][hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case WActionScope::Window:
    {
      bool bExisted = true;
      auto it = s_WindowActions.FindOrAdd(ref_context.m_pWindow, &bExisted);
      if (!bExisted)
      {
        s_pSignalProxy->connect(ref_context.m_pWindow, &QObject::destroyed, s_pSignalProxy, [ref_context]()
          { s_WindowActions.Remove(ref_context.m_pWindow); });
      }
      QWeakPointer<WQtProxy> pTemp = it.Value()[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(ref_context);
        pProxy = QSharedPointer<WQtProxy>(WQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        W_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        it.Value()[hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }
  }

  // make sure we don't use actions that are meant for a different document
  if (pProxy != nullptr && pProxy->GetAction()->GetContext().m_pDocument != nullptr)
  {
    // if this assert fires, you might have tried to map an action into multiple documents, which uses WActionScope::Global
    WAction* pAction = pProxy->GetAction();
    const WActionContext& ctxt = pAction->GetContext();
    WDocument* pDoc = ctxt.m_pDocument;
    W_ASSERT_DEV(pDoc == ref_context.m_pDocument, "invalid document pointer");
  }
  return pProxy;
}

WQtProxy::WQtProxy()
{
  m_pAction = nullptr;
}

WQtProxy::~WQtProxy()
{
  if (m_pAction != nullptr)
    WActionManager::GetActionDescriptor(m_pAction->GetDescriptorHandle())->DeleteAction(m_pAction);
}

void WQtProxy::SetAction(WAction* pAction)
{
  m_pAction = pAction;
}

//////////////////// WQtMenuProxy /////////////////////

WQtMenuProxy::WQtMenuProxy()
{
  m_pMenu = nullptr;
}

WQtMenuProxy::~WQtMenuProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(WMakeDelegate(&WQtMenuProxy::StatusUpdateEventHandler, this));

  m_pMenu->deleteLater();
  delete m_pMenu;
}

void WQtMenuProxy::StatusUpdateEventHandler(WAction* pAction)
{
  Update();
}

void WQtMenuProxy::Update()
{
  auto pMenu = static_cast<WMenuAction*>(m_pAction);

  // note that setting the icon on the menu is pretty pointless, because you'd need to set the icon on something like a QToolButton.
  // therefore there is another event handler in WQtToolBarActionMapView::CreateView()
  m_pMenu->setIcon(WQtUiServices::GetCachedIconResource(pMenu->GetIconPath()));
  m_pMenu->setTitle(WMakeQString(WTranslate(pMenu->GetName())));
  m_pMenu->setToolTip(WMakeQString(WTranslateTooltip(pMenu->GetName())));
}

void WQtMenuProxy::SetAction(WAction* pAction)
{
  WQtProxy::SetAction(pAction);

  m_pMenu = new QMenu();
  m_pMenu->setToolTipsVisible(true);
  Update();

  m_pAction->m_StatusUpdateEvent.AddEventHandler(WMakeDelegate(&WQtMenuProxy::StatusUpdateEventHandler, this));
}

QMenu* WQtMenuProxy::GetQMenu()
{
  return m_pMenu;
}

//////////////////////////////////////////////////////////////////////////
//////////////////// WQtButtonProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

WQtButtonProxy::WQtButtonProxy()
{
  m_pQtAction = nullptr;
}

WQtButtonProxy::~WQtButtonProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(WMakeDelegate(&WQtButtonProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void WQtButtonProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<WButtonAction*>(m_pAction);


  const WActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(pDesc->m_sShortcut.GetData())));

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString sTooltip = WMakeQString(WTranslateTooltip(pButton->GetName()));

  WStringBuilder sDisplay = WTranslate(pButton->GetName());

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  if (!pButton->GetAdditionalDisplayString().IsEmpty())
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  m_pQtAction->setIcon(WQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(QString::fromUtf8(sDisplay.GetData()));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setCheckable(pButton->IsCheckable());
  m_pQtAction->setChecked(pButton->IsChecked());
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void SetupQAction(WAction* pAction, QPointer<QAction>& ref_pQtAction, QObject* pTarget)
{
  WActionDescriptorHandle hDesc = pAction->GetDescriptorHandle();
  const WActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (ref_pQtAction == nullptr)
  {
    ref_pQtAction = new QAction(nullptr);
    W_VERIFY(QObject::connect(ref_pQtAction, SIGNAL(triggered(bool)), pTarget, SLOT(OnTriggered())) != nullptr, "connection failed");

    switch (pDesc->m_Scope)
    {
      case WActionScope::Global:
      {
        // Parent is null so the global actions don't get deleted.
        ref_pQtAction->setShortcutContext(Qt::ShortcutContext::ApplicationShortcut);
      }
      break;
      case WActionScope::Document:
      {
        if (pAction->GetContext().m_pDocument)
        {
          // Parent is set to the window belonging to the document.
          WQtDocumentWindow* pWindow = WQtDocumentWindow::FindWindowByDocument(pAction->GetContext().m_pDocument);
          W_ASSERT_DEBUG(pWindow != nullptr, "You can't map a WActionScope::Document action without that document existing!");
          ref_pQtAction->setParent(pWindow);
          ref_pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
        }
      }
      break;
      case WActionScope::Window:
      {
        ref_pQtAction->setParent(pAction->GetContext().m_pWindow);
        ref_pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
    }
  }
}

void WQtButtonProxy::SetAction(WAction* pAction)
{
  W_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  WQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(WMakeDelegate(&WQtButtonProxy::StatusUpdateEventHandler, this));

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* WQtButtonProxy::GetQAction()
{
  return m_pQtAction;
}

void WQtButtonProxy::StatusUpdateEventHandler(WAction* pAction)
{
  Update();
}

void WQtButtonProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(m_pQtAction->isChecked());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void WQtDynamicMenuProxy::SetAction(WAction* pAction)
{
  WQtMenuProxy::SetAction(pAction);

  W_VERIFY(connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(SlotMenuAboutToShow())) != nullptr, "signal/slot connection failed");
}

void WQtDynamicMenuProxy::SlotMenuAboutToShow()
{
  m_pMenu->clear();

  static_cast<WDynamicMenuAction*>(m_pAction)->GetEntries(m_Entries);

  if (m_Entries.IsEmpty())
  {
    m_pMenu->addAction("<empty>")->setEnabled(false);
  }
  else
  {
    for (WUInt32 i = 0; i < m_Entries.GetCount(); ++i)
    {
      const auto& p = m_Entries[i];

      if (p.m_ItemFlags.IsSet(WDynamicMenuAction::Item::ItemFlags::Separator))
      {
        m_pMenu->addSeparator();
      }
      else
      {
        auto pAction = m_pMenu->addAction(WMakeQString(p.m_sDisplay));
        pAction->setData(i);
        pAction->setIcon(p.m_Icon);
        pAction->setCheckable(p.m_CheckState != WDynamicMenuAction::Item::CheckMark::NotCheckable);
        pAction->setChecked(p.m_CheckState == WDynamicMenuAction::Item::CheckMark::Checked);

        W_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuEntryTriggered())) != nullptr, "signal/slot connection failed");
      }
    }
  }
}

void WQtDynamicMenuProxy::SlotMenuEntryTriggered()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  if (!pAction)
    return;

  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  WUInt32 index = pAction->data().toUInt();
  m_pAction->Execute(m_Entries[index].m_UserValue);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

//////////////////////////////////////////////////////////////////////////
//////////////////// WQtDynamicActionAndMenuProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

WQtDynamicActionAndMenuProxy::WQtDynamicActionAndMenuProxy()
{
  m_pQtAction = nullptr;
}

WQtDynamicActionAndMenuProxy::~WQtDynamicActionAndMenuProxy()
{
  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}


void WQtDynamicActionAndMenuProxy::Update()
{
  WQtDynamicMenuProxy::Update();

  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<WDynamicActionAndMenuAction*>(m_pAction);

  const WActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(pDesc->m_sShortcut.GetData())));

  WStringBuilder sDisplay = WTranslate(pButton->GetName());

  if (!pButton->GetAdditionalDisplayString().IsEmpty())
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString sTooltip = WMakeQString(WTranslateTooltip(pButton->GetName()));

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  m_pQtAction->setIcon(WQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(QString::fromUtf8(sDisplay.GetData()));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void WQtDynamicActionAndMenuProxy::SetAction(WAction* pAction)
{
  WQtDynamicMenuProxy::SetAction(pAction);

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* WQtDynamicActionAndMenuProxy::GetQAction()
{
  return m_pQtAction;
}

void WQtDynamicActionAndMenuProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(WVariant());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

//////////////////////////////////////////////////////////////////////////
//////////////////// WQtSliderProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

WQtSliderWidgetAction::WQtSliderWidgetAction(QWidget* pParent)
  : QWidgetAction(pParent)
{
}

WQtLabeledSlider::WQtLabeledSlider(QWidget* pParent)
  : QWidget(pParent)
{
  m_pLabel = new QLabel(this);
  m_pSlider = new QSlider(this);
  setLayout(new QHBoxLayout(this));

  layout()->addWidget(m_pLabel);
  layout()->addWidget(m_pSlider);

  setMaximumWidth(300);
}

void WQtSliderWidgetAction::setMinimum(int value)
{
  m_iMinimum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    WQtLabeledSlider* pGroup = qobject_cast<WQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMinimum(m_iMinimum);
  }
}

void WQtSliderWidgetAction::setMaximum(int value)
{
  m_iMaximum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    WQtLabeledSlider* pGroup = qobject_cast<WQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMaximum(m_iMaximum);
  }
}

void WQtSliderWidgetAction::setValue(int value)
{
  m_iValue = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    WQtLabeledSlider* pGroup = qobject_cast<WQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setValue(m_iValue);
  }
}

void WQtSliderWidgetAction::OnValueChanged(int value)
{
  Q_EMIT valueChanged(value);
}

QWidget* WQtSliderWidgetAction::createWidget(QWidget* parent)
{
  WQtLabeledSlider* pGroup = new WQtLabeledSlider(parent);
  pGroup->m_pSlider->setOrientation(Qt::Orientation::Horizontal);

  W_VERIFY(connect(pGroup->m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");

  pGroup->m_pLabel->setText(text());
  pGroup->m_pLabel->installEventFilter(this);
  pGroup->m_pLabel->setToolTip(toolTip());
  pGroup->installEventFilter(this);
  pGroup->m_pSlider->setMinimum(m_iMinimum);
  pGroup->m_pSlider->setMaximum(m_iMaximum);
  pGroup->m_pSlider->setValue(m_iValue);
  pGroup->m_pSlider->setToolTip(toolTip());

  return pGroup;
}

bool WQtSliderWidgetAction::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::MouseButtonPress || e->type() == QEvent::Type::MouseButtonRelease || e->type() == QEvent::Type::MouseButtonDblClick)
  {
    e->accept();
    return true;
  }

  return false;
}

WQtSliderProxy::WQtSliderProxy()
{
  m_pQtAction = nullptr;
}

WQtSliderProxy::~WQtSliderProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(WMakeDelegate(&WQtSliderProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void WQtSliderProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pAction = static_cast<WSliderAction*>(m_pAction);

  const WActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();

  WQtSliderWidgetAction* pSliderAction = qobject_cast<WQtSliderWidgetAction*>(m_pQtAction);
  WQtScopedBlockSignals bs(pSliderAction);

  WInt32 minVal, maxVal;
  pAction->GetRange(minVal, maxVal);
  pSliderAction->setMinimum(minVal);
  pSliderAction->setMaximum(maxVal);
  pSliderAction->setValue(pAction->GetValue());
  pSliderAction->setText(WMakeQString(WTranslate(pAction->GetName())));
  pSliderAction->setToolTip(WMakeQString(WTranslateTooltip(pAction->GetName())));
  pSliderAction->setEnabled(pAction->IsEnabled());
  pSliderAction->setVisible(pAction->IsVisible());
}

void WQtSliderProxy::SetAction(WAction* pAction)
{
  W_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  WQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(WMakeDelegate(&WQtSliderProxy::StatusUpdateEventHandler, this));

  WActionDescriptorHandle hDesc = m_pAction->GetDescriptorHandle();
  const WActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (m_pQtAction == nullptr)
  {
    m_pQtAction = new WQtSliderWidgetAction(nullptr);

    W_VERIFY(connect(m_pQtAction, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");
  }

  Update();
}

QAction* WQtSliderProxy::GetQAction()
{
  return m_pQtAction;
}


void WQtSliderProxy::OnValueChanged(int value)
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  // make sure all instances of the slider get updated, by setting the new value
  m_pQtAction->setValue(value);
  m_pAction->Execute(value);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void WQtSliderProxy::StatusUpdateEventHandler(WAction* pAction)
{
  Update();
}
