#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <Foundation/Containers/Deque.h>
#include <QAbstractListModel>

enum class WFileserveActivityType
{
  StartServer,
  StopServer,
  ClientConnect,
  ClientReconnected,
  ClientDisconnect,
  Mount,
  MountFailed,
  Unmount,
  ReadFile,
  WriteFile,
  DeleteFile,
  Other
};

struct WQtFileserveActivityItem
{
  QString m_Text;
  WFileserveActivityType m_Type;
};

class W_EDITORPLUGINFILESERVE_DLL WQtFileserveActivityModel : public QAbstractListModel
{
  Q_OBJECT

public:
  WQtFileserveActivityModel(QWidget* pParent);

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int iRole = Qt::DisplayRole) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;

  WQtFileserveActivityItem& AppendItem();
  void UpdateView();

  void Clear();
private Q_SLOTS:
  void UpdateViewSlot();

private:
  bool m_bTimerRunning = false;
  WUInt32 m_uiAddedItems = 0;
  WDeque<WQtFileserveActivityItem> m_Items;
};
