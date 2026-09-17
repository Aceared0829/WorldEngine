#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QCloseEvent>
#include <QLabel>
#include <QStatusBar>
#include <QTabBar>
#include <ads/DockAreaWidget.h>
#include <ads/DockManager.h>
#include <ads/DockWidgetTab.h>
#include <ads/FloatingDockContainer.h>

WQtContainerWindow* WQtContainerWindow::s_pContainerWindow = nullptr;
bool WQtContainerWindow::s_bForceClose = false;

WQtContainerWindow::WQtContainerWindow()
{
  setMinimumSize(QSize(800, 600));
  m_pStatusBarLabel = nullptr;

  s_pContainerWindow = this;

  setObjectName("WEditor");
  setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/W-logo.svg")));

  WQtDocumentWindow::s_Events.AddEventHandler(WMakeDelegate(&WQtContainerWindow::DocumentWindowEventHandler, this));
  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WQtContainerWindow::ProjectEventHandler, this));
  WQtUiServices::s_Events.AddEventHandler(WMakeDelegate(&WQtContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  ads::CDockManager::ConfigFlags flags =
    ads::CDockManager::DefaultDockAreaButtons |
    ads::CDockManager::ActiveTabHasCloseButton |
    ads::CDockManager::XmlCompressionEnabled |
    ads::CDockManager::FloatingContainerHasWidgetTitle |
    ads::CDockManager::FloatingContainerHasWidgetIcon |
    ads::CDockManager::HideSingleCentralWidgetTitleBar |
    ads::CDockManager::DragPreviewShowsContentPixmap |
    // ads::CDockManager::FocusHighlighting |
    // ads::CDockManager::AlwaysShowTabs |
    // ads::CDockManager::DockAreaHasCloseButton |
    ads::CDockManager::DockAreaCloseButtonClosesTab |
    ads::CDockManager::MiddleMouseButtonClosesTab |
    ads::CDockManager::DockAreaHasTabsMenuButton |
    ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility |
    // ads::CDockManager::AllTabsHaveCloseButton |
    ads::CDockManager::RetainTabSizeWhenCloseButtonHidden |
    ads::CDockManager::DockAreaHideDisabledButtons |
    ads::CDockManager::DockAreaHasUndockButton |
    // ads::CDockManager::DoubleClickUndocksWidget | // don't want this
    ads::CDockManager::OpaqueSplitterResize;
  ads::CDockManager::setConfigFlags(flags);

  ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);
  ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideShowOnMouseOver, false);
  ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideCloseOnOutsideMouseClick, false);

  m_pDockManager = new ads::CDockManager(this);

  connect(m_pDockManager, &ads::CDockManager::floatingWidgetCreated, this, &WQtContainerWindow::SlotFloatingWidgetOpened);
}

WQtContainerWindow::~WQtContainerWindow()
{
  s_pContainerWindow = nullptr;

  WQtDocumentWindow::s_Events.RemoveEventHandler(WMakeDelegate(&WQtContainerWindow::DocumentWindowEventHandler, this));
  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WQtContainerWindow::ProjectEventHandler, this));
  WQtUiServices::s_Events.RemoveEventHandler(WMakeDelegate(&WQtContainerWindow::UIServicesEventHandler, this));
}

void WQtContainerWindow::UpdateWindowTitle()
{
  WStringBuilder sTitle;

  if (WToolsProject::IsProjectOpen())
  {
    sTitle = WToolsProject::GetSingleton()->GetProjectName(false);
    sTitle.Append(" - ");
  }

  sTitle.Append(WApplication::GetApplicationInstance()->GetApplicationName().GetView());

  setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void WQtContainerWindow::closeEvent(QCloseEvent* e)
{
  if (s_bForceClose)
    return;

  s_bForceClose = true;
  W_SCOPE_EXIT(s_bForceClose = false);

  e->setAccepted(true);

  if (!WToolsProject::CanCloseProject())
  {
    e->setAccepted(false);
    return;
  }

  WToolsProject::SaveProjectState();

  // do not close the documents in the main container window here,
  // as that would remove them from the recently-open documents list and not restore them when opening the editor again
  WDynamicArray<WQtDocumentWindow*> windows = m_DocumentWindows;
  for (WQtDocumentWindow* pWindow : windows)
  {
    pWindow->ShutdownDocumentWindow();
  }

  // We need to destroy the dock manager here, doing it in the constructor leads to an access violation.
  m_pDockManager->deleteLater();
  m_pDockManager = nullptr;
  QMainWindow::closeEvent(e);
}

void WQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<WQtDocumentWindow*>(pDocWindow));
}

void WQtContainerWindow::SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget)
{
  FloatingWidget->installEventFilter(this);
}

void WQtContainerWindow::SlotDockWidgetFloatingChanged(bool bFloating)
{
  if (!bFloating)
    return;

  for (auto pDoc : m_DocumentWindows)
  {
    UpdateWindowDecoration(pDoc);
  }
}

void WQtContainerWindow::UpdateWindowDecoration(WQtDocumentWindow* pDocWindow)
{
  const WUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == WInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->setTabToolTip(QString::fromUtf8(pDocWindow->GetDisplayName().GetData()));
  dock->setIcon(WQtUiServices::GetCachedIconResource(pDocWindow->GetWindowIcon().GetData()));
  dock->setWindowTitle(QString::fromUtf8(pDocWindow->GetDisplayNameShort().GetData()));

  if (dock->isFloating())
  {
    dock->dockContainer()->floatingWidget()->setWindowTitle(dock->windowTitle());
    dock->dockContainer()->floatingWidget()->setWindowIcon(dock->icon());
  }
}

void WQtContainerWindow::RemoveDocumentWindow(WQtDocumentWindow* pDocWindow)
{
  const WUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == WInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  int iCurIdx = -1;

  const bool bIsTabbed = dock->isTabbed();
  ads::CDockAreaWidget* pDockArea = dock->dockAreaWidget();

  iCurIdx = pDockArea->currentIndex();

  m_pDockManager->removeDockWidget(dock);

  m_DocumentWindows.RemoveAtAndSwap(uiListIndex);
  m_DocumentDocks.RemoveAtAndSwap(uiListIndex);
  W_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());
  dock->hide();
  dock->deleteLater();
  pDocWindow->m_pContainerWindow = nullptr;

  if (bIsTabbed)
  {
    iCurIdx = WMath::Min(iCurIdx, pDockArea->openDockWidgetsCount() - 1);
    pDockArea->setCurrentIndex(iCurIdx);
    pDockArea->currentDockWidget()->update();
  }

  if (pDockArea && pDockArea->openDockWidgetsCount() == 1)
  {
    for (auto pDocWindow2 : m_DocumentWindows)
    {
      UpdateWindowDecoration(pDocWindow2);
    }
  }
}

void WQtContainerWindow::RemoveApplicationPanel(WQtApplicationPanel* pPanel)
{
  const auto uiListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (uiListIndex == WInvalidIndex)
    return;

  m_pDockManager->removeDockWidget(pPanel);
  m_ApplicationPanels.RemoveAtAndSwap(uiListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void WQtContainerWindow::AddDocumentWindow(WQtDocumentWindow* pDocWindow)
{
  W_PROFILE_SCOPE("AddDocumentWindow");
  W_ASSERT_DEV(!pDocWindow->objectName().isEmpty(), "Panel name must be unique and not empty.");

  if (m_DocumentWindows.IndexOf(pDocWindow) != WInvalidIndex)
    return;

  W_ASSERT_DEV(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  // NOTE: This function is called by the WQtDocumentWindow constructor
  // that means any derived classes are not yet constructed!
  // therefore calling virtual functions here, like GetDisplayNameShort() will still call
  // the base class implementation, NOT the derived one !
  // therefore, we do some stuff in WQtContainerWindow::UpdateWindowDecoration() instead

  pDocWindow->m_pContainerWindow = this;

  m_DocumentWindows.PushBack(pDocWindow);
  WString displayName = pDocWindow->GetDisplayNameShort();
  ads::CDockWidget* dock = new ads::CDockWidget(m_pDockManager, QString::fromUtf8(displayName.GetData(), displayName.GetElementCount()));
  dock->installEventFilter(pDocWindow);

  dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
  dock->setFeature(ads::CDockWidget::DockWidgetPinnable, false);

  // this is a hacky way to detect the WQtSettingsTab
  if (displayName == "Settings")
  {
    dock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    dock->setFeature(ads::CDockWidget::NoTab, true);
  }

  dock->setObjectName(pDocWindow->GetUniqueName());
  W_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  W_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
  dock->setWidget(pDocWindow);
  dock->tabWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
  if (!m_DocumentDocks.IsEmpty())
  {
    ads::CDockAreaWidget* dockArea = m_DocumentDocks.PeekBack()->dockAreaWidget();
    m_pDockManager->addDockWidgetTabToArea(dock, dockArea);
  }
  else
  {
    W_PROFILE_SCOPE("AddDocumentWindow - addDockWidgetTab");
    m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, dock);
  }
  m_DocumentDocks.PushBack(dock);
  connect(dock, &ads::CDockWidget::closeRequested, this, &WQtContainerWindow::SlotDocumentTabCloseRequested);
  connect(dock->tabWidget(), &QWidget::customContextMenuRequested, this, &WQtContainerWindow::SlotTabsContextMenuRequested);
  connect(dock, &ads::CDockWidget::topLevelChanged, this, &WQtContainerWindow::SlotDockWidgetFloatingChanged);


  pDocWindow->m_pContainerWindow = this;

  // we cannot call virtual functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void WQtContainerWindow::DocumentWindowRenamed(WQtDocumentWindow* pDocWindow)
{
  const WUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == WInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];
  W_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());

  dock->setObjectName(pDocWindow->GetUniqueName());
  W_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  W_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
}

void WQtContainerWindow::AddApplicationPanel(WQtApplicationPanel* pPanel)
{
  // panel already in container window ?
  if (m_ApplicationPanels.IndexOf(pPanel) != WInvalidIndex)
    return;

  W_ASSERT_DEV(!pPanel->objectName().isEmpty(), "Dock name must not be empty.");
  W_ASSERT_DEV(!m_DockNames.contains(pPanel->objectName()), "Dock name must be unique.");
  m_DockNames.insert(pPanel->objectName());
  W_ASSERT_DEV(pPanel->m_pContainerWindow == nullptr, "Implementation error");

  m_ApplicationPanels.PushBack(pPanel);
  pPanel->m_pContainerWindow = this;
  m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
}

WResult WQtContainerWindow::EnsureVisible(WQtDocumentWindow* pDocWindow)
{
  const auto uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (uiListIndex == WInvalidIndex)
    return W_FAILURE;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->toggleView(true);
  return W_SUCCESS;
}

WResult WQtContainerWindow::EnsureVisible(WDocument* pDocument)
{
  for (auto doc : m_DocumentWindows)
  {
    if (doc->GetDocument() == pDocument)
      return EnsureVisible(doc);
  }

  return W_FAILURE;
}

WResult WQtContainerWindow::EnsureVisible(WQtApplicationPanel* pPanel)
{
  if (m_ApplicationPanels.IndexOf(pPanel) == WInvalidIndex)
    return W_FAILURE;

  if (pPanel->isClosed())
  {
    pPanel->toggleView();
  }
  pPanel->raise();
  return W_SUCCESS;
}

void WQtContainerWindow::SaveDocumentWindowStates(WMap<ads::CDockWidget*, DocumentWindowState>& out_states)
{
  out_states.Clear();
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    DocumentWindowState state;
    state.m_bFloating = pDock->isFloating();
    out_states[pDock] = state;
  }
}

void WQtContainerWindow::RestoreDocumentWindowStates(const WMap<ads::CDockWidget*, DocumentWindowState>& states)
{
  if (m_DocumentDocks.IsEmpty())
    return;

  // Find a dock area where documents are docked (not floating, not closed)
  ads::CDockAreaWidget* pDocumentArea = nullptr;
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    if (!pDock->isClosed() && !pDock->isFloating())
    {
      pDocumentArea = pDock->dockAreaWidget();
      break;
    }
  }

  // Restore each document window to its previous state
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    auto it = states.Find(pDock);
    const bool bWasFloating = it.IsValid() ? it.Value().m_bFloating : false;

    if (pDock->isClosed())
    {
      if (bWasFloating)
      {
        // Was floating, make it floating again
        m_pDockManager->addDockWidgetFloating(pDock);
      }
      else
      {
        // Was docked, re-dock it
        if (pDocumentArea != nullptr)
        {
          m_pDockManager->addDockWidgetTabToArea(pDock, pDocumentArea);
        }
        else
        {
          // No area yet, add to center and use that as the document area
          pDocumentArea = m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, pDock);
        }
      }
    }
    // If not closed, leave it as-is (it's already visible, either floating or docked)
  }

  for (auto pDocWindow : m_DocumentWindows)
  {
    UpdateWindowDecoration(pDocWindow);
  }
}

WResult WQtContainerWindow::EnsureVisibleAnyContainer(WDocument* pDocument)
{
  // make sure there is a window to make visible in the first place
  pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  if (s_pContainerWindow->EnsureVisible(pDocument).Succeeded())
    return W_SUCCESS;

  return W_FAILURE;
}

void WQtContainerWindow::GetDocumentWindows(WHybridArray<WQtDocumentWindow*, 16>& ref_windows)
{
  ref_windows = m_DocumentWindows;
}

bool WQtContainerWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::Close)
  {
    if (auto* pFloatingWidget = qobject_cast<ads::CFloatingDockContainer*>(obj))
    {
      WTempHybridArray<WDocument*, 32> docs;
      docs.Reserve(m_DocumentWindows.GetCount());
      WTempHybridArray<WQtDocumentWindow*, 32> windows;
      windows.Reserve(m_DocumentWindows.GetCount());

      QList<ads::CDockWidget*> floatingDocks = pFloatingWidget->dockWidgets();
      for (WUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
      {
        if (floatingDocks.contains(m_DocumentDocks[i]))
        {
          docs.PushBack(m_DocumentWindows[i]->GetDocument());
          windows.PushBack(m_DocumentWindows[i]);
        }
      }

      if (!WToolsProject::CanCloseDocuments(docs))
      {
        e->setAccepted(false);
        return true;
      }

      // closing a non-main window should close all documents as well
      // this will remove them from the recently-open documents list and not restore them next time
      for (WQtDocumentWindow* pWindow : windows)
      {
        pWindow->CloseDocumentWindow();
      }
      // This is necessary to clean up some 'delete later' Qt objects before the document is closed as they need to remove their references to the doc.
      qApp->processEvents();
    }
  }
  return false;
}

void WQtContainerWindow::SlotDocumentTabCloseRequested()
{
  auto dock = qobject_cast<ads::CDockWidget*>(sender());
  const auto uiListIndex = m_DocumentDocks.IndexOf(dock);
  W_ASSERT_DEV(uiListIndex != WInvalidIndex, "Can't close non-existing document.");

  WQtDocumentWindow* pDocWindow = m_DocumentWindows[uiListIndex];

  if (!pDocWindow->CanCloseWindow())
  {
    return;
  }

  pDocWindow->CloseDocumentWindow();
}

void WQtContainerWindow::DocumentWindowEventHandler(const WQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
    case WQtDocumentWindowEvent::Type::WindowClosing:
      RemoveDocumentWindow(e.m_pWindow);
      break;
    case WQtDocumentWindowEvent::Type::WindowDecorationChanged:
      UpdateWindowDecoration(e.m_pWindow);
      break;

    default:
      break;
  }
}

void WQtContainerWindow::ProjectEventHandler(const WToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case WToolsProjectEvent::Type::ProjectOpened:
    case WToolsProjectEvent::Type::ProjectClosed:
      UpdateWindowTitle();
      break;

    default:
      break;
  }
}

void WQtContainerWindow::UIServicesEventHandler(const WQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case WQtUiServices::Event::Type::ShowGlobalStatusBarText:
    {
      if (statusBar() == nullptr)
        setStatusBar(new QStatusBar());

      if (m_pStatusBarLabel == nullptr)
      {
        m_pStatusBarLabel = new QLabel();
        statusBar()->addWidget(m_pStatusBarLabel);

        QPalette pal = m_pStatusBarLabel->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::red));
        m_pStatusBarLabel->setPalette(pal);
      }

      statusBar()->setHidden(e.m_sText.IsEmpty());
      m_pStatusBarLabel->setText(QString::fromUtf8(e.m_sText.GetData()));
    }
    break;

    default:
      break;
  }
}

void WQtContainerWindow::SlotTabsContextMenuRequested(const QPoint& pos)
{
  auto tab = qobject_cast<ads::CDockWidgetTab*>(sender());
  ads::CDockWidget* dock = tab->dockWidget();
  const auto uiListIndex = m_DocumentDocks.IndexOf(dock);
  W_ASSERT_DEV(uiListIndex != WInvalidIndex, "Can't close non-existing document.");

  WQtDocumentWindow* pDoc = m_DocumentWindows[uiListIndex];
  pDoc->RequestWindowTabContextMenu(tab->mapToGlobal(pos));
}
