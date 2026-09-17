#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_FileWidget.h>
#include <ads/DockWidget.h>

class WQtFileWidget : public ads::CDockWidget, public Ui_FileWidget
{
public:
  Q_OBJECT

public:
  WQtFileWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);
  ~WQtFileWidget();
  static WQtFileWidget* s_pWidget;

private Q_SLOTS:

  virtual void on_SpinLimitToRecent_valueChanged(int val);
  virtual void on_SpinMinDuration_valueChanged(double val);
  virtual void on_LineFilterByName_textChanged();
  virtual void on_ComboThread_currentIndexChanged(int state);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  enum FileOpState
  {
    None,
    OpenReading,
    OpenWriting,
    ClosedReading,
    ClosedWriting,
    OpenReadingFailed,
    OpenWritingFailed,
    FileExists,
    FileExistsFailed,
    FileDelete,
    FileDeleteFailed,
    FileCopy,
    FileCopyFailed,
    CreateDirs,
    CreateDirsFailed,
    FileStat,
    FileStatFailed,
    FileCasing,
    FileCasingFailed,
  };

  struct FileOpData
  {
    WString m_sFile;
    FileOpState m_State;
    WTime m_StartTime;
    WTime m_BlockedDuration;
    WUInt64 m_uiBytesAccessed;
    WUInt8 m_uiThreadTypes; // 1 = Main, 2 = Task: Loading, 4 = Other

    FileOpData()
    {
      m_State = None;
      m_uiBytesAccessed = 0;
      m_uiThreadTypes = 0;
    }
  };

  QTableWidgetItem* GetStateString(FileOpState State) const;

  WInt32 m_iMaxID;
  WTime m_LastTableUpdate;
  bool m_bUpdateTable;
  WHashTable<WUInt32, FileOpData> m_FileOps;
};
