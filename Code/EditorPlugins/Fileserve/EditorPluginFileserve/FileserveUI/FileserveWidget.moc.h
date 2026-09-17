#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <EditorPluginFileserve/ui_FileserveWidget.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Time.h>
#include <QWidget>

struct WFileserverEvent;
class WQtFileserveActivityModel;
class WQtFileserveAllFilesModel;
enum class WFileserveActivityType;

/// A GUI for the WFileServer
///
/// By default the file server does run at startup. Using the command line option "-fs_nostart" prevents that.
class W_EDITORPLUGINFILESERVE_DLL WQtFileserveWidget : public QWidget, public Ui_WQtFileserveWidget
{
  Q_OBJECT

public:
  WQtFileserveWidget(QWidget* pParent = nullptr);

  void FindOwnIP(WStringBuilder& out_sDisplay, WHybridArray<WStringBuilder, 4>* out_pAllIPs = nullptr);

  ~WQtFileserveWidget();

Q_SIGNALS:
  void ServerStarted(const QString& sIp, WUInt16 uiPort);
  void ServerStopped();

public Q_SLOTS:
  void on_StartServerButton_clicked();
  void on_ClearActivityButton_clicked();
  void on_ClearAllFilesButton_clicked();
  void on_ReloadResourcesButton_clicked();
  void on_ConnectClient_clicked();

private:
  void FileserverEventHandler(const WFileserverEvent& e);
  void LogActivity(const WFormatString& text, WFileserveActivityType type);
  void UpdateSpecialDirectoryUI();

  WQtFileserveActivityModel* m_pActivityModel;
  WQtFileserveAllFilesModel* m_pAllFilesModel;
  WTime m_LastProgressUpdate;

  struct DataDirInfo
  {
    WString m_sName;
    WString m_sPath;
    WString m_sRedirectedPath;
  };

  struct ClientData
  {
    bool m_bConnected = false;
    WHybridArray<DataDirInfo, 8> m_DataDirs;
  };

  struct SpecialDir
  {
    WString m_sName;
    WString m_sPath;
  };

  WHybridArray<SpecialDir, 4> m_SpecialDirectories;

  WHashTable<WUInt32, ClientData> m_Clients;
  void UpdateClientList();
  void ConfigureSpecialDirectories();
};
