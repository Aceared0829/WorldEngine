#include <Inspector/InspectorPCH.h>

#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/RenderGraphWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

const int g_iDockingStateVersion = 1;

WQtMainWindow* WQtMainWindow::s_pWidget = nullptr;

WQtMainWindow::WQtMainWindow()
  : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

  m_DockManager = new ads::CDockManager(this);
  m_DockManager->setConfigFlags(
    static_cast<ads::CDockManager::ConfigFlags>(ads::CDockManager::DockAreaHasCloseButton | ads::CDockManager::DockAreaCloseButtonClosesTab |
                                                ads::CDockManager::OpaqueSplitterResize | ads::CDockManager::AllTabsHaveCloseButton));

  QSettings Settings;
  m_sConnectionTarget = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();
  SetAlwaysOnTop((OnTopMode)Settings.value("AlwaysOnTop", (int)WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  const bool bRestoreDockingState = Settings.value("DockingVersion") == g_iDockingStateVersion;

  if (bRestoreDockingState)
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
  }

  // The dock manager will set ownership to null on add so there is no reason to provide an owner here.
  // Setting one will actually cause memory corruptions on shutdown for unknown reasons.
  WQtMainWidget* pMainWidget = new WQtMainWidget(m_DockManager);
  WQtLogDockWidget* pLogWidget = new WQtLogDockWidget(m_DockManager);
  WQtMemoryWidget* pMemoryWidget = new WQtMemoryWidget(m_DockManager);
  WQtTimeWidget* pTimeWidget = new WQtTimeWidget(m_DockManager);
  WQtInputWidget* pInputWidget = new WQtInputWidget(m_DockManager);
  WQtCVarsWidget* pCVarsWidget = new WQtCVarsWidget(m_DockManager);
  WQtSubsystemsWidget* pSubsystemsWidget = new WQtSubsystemsWidget(m_DockManager);
  WQtFileWidget* pFileWidget = new WQtFileWidget(m_DockManager);
  WQtPluginsWidget* pPluginsWidget = new WQtPluginsWidget(m_DockManager);
  WQtGlobalEventsWidget* pGlobalEventesWidget = new WQtGlobalEventsWidget(m_DockManager);
  WQtReflectionWidget* pReflectionWidget = new WQtReflectionWidget(m_DockManager);
  WQtDataWidget* pDataWidget = new WQtDataWidget(m_DockManager);
  WQtResourceWidget* pResourceWidget = new WQtResourceWidget(m_DockManager);
  WQtRenderGraphWidget* pRenderGraphWidget = new WQtRenderGraphWidget(m_DockManager);

  W_VERIFY(nullptr != QWidget::connect(pMainWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pLogWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pTimeWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pMemoryWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pInputWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pCVarsWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pReflectionWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pFileWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pPluginsWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(
    nullptr != QWidget::connect(pGlobalEventesWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pDataWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pResourceWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");
  W_VERIFY(nullptr != QWidget::connect(pRenderGraphWidget, &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.svg"));

  for (WUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new WQtStatVisWidget(m_DockManager, this, i);
    m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);

    W_VERIFY(
      nullptr != QWidget::connect(m_pStatHistoryWidgets[i], &ads::CDockWidget::viewToggled, this, &WQtMainWindow::DockWidgetVisibilityChanged), "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pActionShowStatIn[i] = new QAction(this);

    W_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i], &QAction::triggered, WQtMainWidget::s_pWidget, &WQtMainWidget::ShowStatIn), "");
  }

  // delay this until after all widgets are created
  for (WUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->toggleView(false); // hide
  }

  setContextMenuPolicy(Qt::NoContextMenu);

  menuWindows->addMenu(pHistoryMenu);

  pMemoryWidget->raise();

  m_DockManager->addDockWidget(ads::LeftDockWidgetArea, pMainWidget);
  m_DockManager->addDockWidget(ads::CenterDockWidgetArea, pLogWidget);

  m_DockManager->addDockWidget(ads::RightDockWidgetArea, pCVarsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pGlobalEventesWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pDataWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pInputWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPluginsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pReflectionWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pResourceWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pRenderGraphWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pSubsystemsWidget);

  m_DockManager->addDockWidget(ads::BottomDockWidgetArea, pFileWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pMemoryWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pTimeWidget);


  pLogWidget->raise();
  pCVarsWidget->raise();

  if (bRestoreDockingState)
  {
    auto dockState = Settings.value("DockManagerState");
    if (dockState.isValid() && dockState.typeId() == QMetaType::QByteArray)
    {
      m_DockManager->restoreState(dockState.toByteArray(), 1);
    }

    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());

    if (Settings.value("IsMaximized", isMaximized()).toBool())
    {
      showMaximized();
    }

    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }

  Settings.endGroup();

  for (WInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->Load();

  UpdateWindowTitle();
  SetupNetworkTimer();
}

WQtMainWindow::~WQtMainWindow()
{
  for (WInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
  }
  // The dock manager does not take ownership of dock widgets.
  auto dockWidgets = m_DockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_DockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void WQtMainWindow::closeEvent(QCloseEvent* pEvent)
{
  const bool bMaximized = isMaximized();
  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("DockingVersion", g_iDockingStateVersion);
  Settings.setValue("DockManagerState", m_DockManager->saveState(1));
  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", bMaximized);
  Settings.setValue("WindowPosition", pos());
  if (!bMaximized)
    Settings.setValue("WindowSize", size());

  Settings.endGroup();
}

void WQtMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == nullptr)
    m_pNetworkTimer = new QTimer(this);

  m_pNetworkTimer->singleShot(40, this, SLOT(UpdateNetworkTimeOut()));
}

void WQtMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void WQtMainWindow::UpdateNetwork()
{
  bool bResetStats = false;

  {
    static WUInt32 uiServerID = 0;

    if (WTelemetry::IsConnectedToServer())
    {
      if (uiServerID != WTelemetry::GetServerID())
      {
        uiServerID = WTelemetry::GetServerID();
        bResetStats = true;

        WStringBuilder s;
        s.SetFormat("Connected to new Server with ID {0}", uiServerID);

        WQtLogDockWidget::s_pWidget->Log(s.GetData());
      }
      else if (!m_bConnectedToServer)
      {
        WQtLogDockWidget::s_pWidget->Log("Reconnected to Server.");
      }

      if (m_sLastServerName != WTelemetry::GetServerName())
      {
        m_sLastServerName = WTelemetry::GetServerName();
        UpdateWindowTitle();
      }

      if (!m_bConnectedToServer)
      {
        m_bConnectedToServer = true;
        UpdateWindowTitle();
      }
    }
    else
    {
      if (m_bConnectedToServer)
      {
        WQtLogDockWidget::s_pWidget->Log("Lost Connection to Server.");
        m_sLastServerName.Clear();
        m_bConnectedToServer = false;
        UpdateWindowTitle();
      }
    }
  }

  if (bResetStats)
  {


    WQtMainWidget::s_pWidget->ResetStats();
    WQtLogDockWidget::s_pWidget->ResetStats();
    WQtMemoryWidget::s_pWidget->ResetStats();
    WQtTimeWidget::s_pWidget->ResetStats();
    WQtInputWidget::s_pWidget->ResetStats();
    WQtCVarsWidget::s_pWidget->ResetStats();
    WQtReflectionWidget::s_pWidget->ResetStats();
    WQtFileWidget::s_pWidget->ResetStats();
    WQtPluginsWidget::s_pWidget->ResetStats();
    WQtSubsystemsWidget::s_pWidget->ResetStats();
    WQtGlobalEventsWidget::s_pWidget->ResetStats();
    WQtDataWidget::s_pWidget->ResetStats();
    WQtResourceWidget::s_pWidget->ResetStats();
    WQtRenderGraphWidget::s_pWidget->ResetStats();
  }

  UpdateAlwaysOnTop();

  WQtMainWidget::s_pWidget->UpdateStats();
  WQtPluginsWidget::s_pWidget->UpdateStats();
  WQtSubsystemsWidget::s_pWidget->UpdateStats();
  WQtMemoryWidget::s_pWidget->UpdateStats();
  WQtTimeWidget::s_pWidget->UpdateStats();
  WQtFileWidget::s_pWidget->UpdateStats();
  WQtResourceWidget::s_pWidget->UpdateStats();
  WQtRenderGraphWidget::s_pWidget->UpdateStats();
  // WQtDataWidget::s_pWidget->UpdateStats();

  for (WInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();

  WTelemetry::PerFrameUpdate();
}

void WQtMainWindow::UpdateWindowTitle()
{
  if (m_bConnectedToServer && !m_sLastServerName.IsEmpty())
    setWindowTitle(QString("WInspector [%1] - %2").arg(m_sConnectionTarget, m_sLastServerName.GetData()));
  else if (m_bConnectedToServer)
    setWindowTitle(QString("WInspector [%1] - connected").arg(m_sConnectionTarget));
  else
    setWindowTitle(QString("WInspector [%1] - not connected").arg(m_sConnectionTarget));
}

void WQtMainWindow::SetConnectionTarget(const QString& sTarget)
{
  m_sConnectionTarget = sTarget;
  UpdateWindowTitle();
}

void WQtMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  // TODO: add menu entry for qt main widget

  ActionShowWindowLog->setChecked(!WQtLogDockWidget::s_pWidget->isClosed());
  ActionShowWindowMemory->setChecked(!WQtMemoryWidget::s_pWidget->isClosed());
  ActionShowWindowTime->setChecked(!WQtTimeWidget::s_pWidget->isClosed());
  ActionShowWindowInput->setChecked(!WQtInputWidget::s_pWidget->isClosed());
  ActionShowWindowCVar->setChecked(!WQtCVarsWidget::s_pWidget->isClosed());
  ActionShowWindowReflection->setChecked(!WQtReflectionWidget::s_pWidget->isClosed());
  ActionShowWindowSubsystems->setChecked(!WQtSubsystemsWidget::s_pWidget->isClosed());
  ActionShowWindowFile->setChecked(!WQtFileWidget::s_pWidget->isClosed());
  ActionShowWindowPlugins->setChecked(!WQtPluginsWidget::s_pWidget->isClosed());
  ActionShowWindowGlobalEvents->setChecked(!WQtGlobalEventsWidget::s_pWidget->isClosed());
  ActionShowWindowData->setChecked(!WQtDataWidget::s_pWidget->isClosed());
  ActionShowWindowResource->setChecked(!WQtResourceWidget::s_pWidget->isClosed());
  ActionShowWindowRenderGraph->setChecked(!WQtRenderGraphWidget::s_pWidget->isClosed());

  for (WInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(!m_pStatHistoryWidgets[i]->isClosed());
}


void WQtMainWindow::SetAlwaysOnTop(OnTopMode Mode)
{
  m_OnTopMode = Mode;

  QSettings Settings;
  Settings.setValue("AlwaysOnTop", (int)m_OnTopMode);

  ActionNeverOnTop->setChecked((m_OnTopMode == Never) ? Qt::Checked : Qt::Unchecked);
  ActionAlwaysOnTop->setChecked((m_OnTopMode == Always) ? Qt::Checked : Qt::Unchecked);
  ActionOnTopWhenConnected->setChecked((m_OnTopMode == WhenConnected) ? Qt::Checked : Qt::Unchecked);

  UpdateAlwaysOnTop();
}

void WQtMainWindow::UpdateAlwaysOnTop()
{
  static bool bOnTop = false;

  bool bNewState = bOnTop;
  W_IGNORE_UNUSED(bNewState);

  if (m_OnTopMode == Always || (m_OnTopMode == WhenConnected && WTelemetry::IsConnectedToServer()))
    bNewState = true;
  else
    bNewState = false;

  if (bOnTop != bNewState)
  {
    bOnTop = bNewState;

    hide();

    if (bOnTop)
      setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
      setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

    show();
  }
}

void WQtMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage(' APP', Msg) == W_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case 'ASRT':
      {
        WString sSourceFile, sFunction, sExpression, sMessage;
        WUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

        WQtLogDockWidget::s_pWidget->Log("");
        WQtLogDockWidget::s_pWidget->Log("<<< Application Assertion >>>");
        WQtLogDockWidget::s_pWidget->Log("");

        WQtLogDockWidget::s_pWidget->Log(WFmt("    Expression: '{0}'", sExpression));
        WQtLogDockWidget::s_pWidget->Log("");

        WQtLogDockWidget::s_pWidget->Log(WFmt("    Message: '{0}'", sMessage));
        WQtLogDockWidget::s_pWidget->Log("");

        WQtLogDockWidget::s_pWidget->Log(WFmt("   File: '{0}'", sSourceFile));

        WQtLogDockWidget::s_pWidget->Log(WFmt("   Line: {0}", uiLine));

        WQtLogDockWidget::s_pWidget->Log(WFmt("   In Function: '{0}'", sFunction));

        WQtLogDockWidget::s_pWidget->Log("");

        WQtLogDockWidget::s_pWidget->Log(">>> Application Assertion <<<");
        WQtLogDockWidget::s_pWidget->Log("");
      }
      break;
    }
  }
}
