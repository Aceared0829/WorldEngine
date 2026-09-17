#pragma once

#include <Fileserve/Fileserve.h>

#ifdef W_USE_QT

#  include <QMainWindow>

class WApplication;
class WQtFileserveWidget;

class WQtFileserveMainWnd : public QMainWindow
{
  Q_OBJECT
public:
  WQtFileserveMainWnd(WApplication* pApp, QWidget* pParent = nullptr);

private Q_SLOTS:
  void UpdateNetworkSlot();
  void OnServerStarted(const QString& ip, WUInt16 uiPort);
  void OnServerStopped();

private:
  WApplication* m_pApp;
  WQtFileserveWidget* m_pFileserveWidget = nullptr;
};

void CreateFileserveMainWindow(WApplication* pApp);

#endif
