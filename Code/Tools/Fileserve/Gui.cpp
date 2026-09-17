#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef W_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>

void CreateFileserveMainWindow(WApplication* pApp)
{
  WQtFileserveMainWnd* pMainWnd = new WQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

WQtFileserveMainWnd::WQtFileserveMainWnd(WApplication* pApp, QWidget* pParent)
  : QMainWindow(pParent)
  , m_pApp(pApp)
{
  OnServerStopped();

  m_pFileserveWidget = new WQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  resize(700, 650);

  connect(m_pFileserveWidget, &WQtFileserveWidget::ServerStarted, this, &WQtFileserveMainWnd::OnServerStarted);
  connect(m_pFileserveWidget, &WQtFileserveWidget::ServerStopped, this, &WQtFileserveMainWnd::OnServerStopped);

  show();

  QTimer::singleShot(0, this, &WQtFileserveMainWnd::UpdateNetworkSlot);

  setWindowIcon(m_pFileserveWidget->windowIcon());
}


void WQtFileserveMainWnd::UpdateNetworkSlot()
{
  m_pApp->Run();

  if (m_pApp->ShouldApplicationQuit())
  {
    close();
  }
  else
  {
    QTimer::singleShot(0, this, &WQtFileserveMainWnd::UpdateNetworkSlot);
  }
}

void WQtFileserveMainWnd::OnServerStarted(const QString& ip, WUInt16 uiPort)
{
  QString title = QString("WFileserve (Port %1)").arg(uiPort);

  setWindowTitle(title);
}

void WQtFileserveMainWnd::OnServerStopped()
{
  setWindowTitle("WFileserve (not running)");
}

#endif
