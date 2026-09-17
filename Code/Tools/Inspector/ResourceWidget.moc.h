#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_ResourceWidget.h>
#include <ads/DockWidget.h>

class WQtResourceWidget : public ads::CDockWidget, public Ui_ResourceWidget
{
public:
  Q_OBJECT

public:
  WQtResourceWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtResourceWidget* s_pWidget;

private Q_SLOTS:

  void on_LineFilterByName_textChanged();
  void on_ComboResourceTypes_currentIndexChanged(int state);
  void on_CheckShowDeleted_toggled(bool checked);
  void on_ButtonSave_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

  void UpdateTable();

private:
  void UpdateAll();

  struct ResourceData
  {
    ResourceData()
    {
      m_pMainItem = nullptr;
      m_bUpdate = true;
    }

    bool m_bUpdate;
    QTableWidgetItem* m_pMainItem;
    WString m_sResourceID;
    WString m_sResourceType;
    WResourcePriority m_Priority;
    WBitflags<WResourceFlags> m_Flags;
    WResourceLoadDesc m_LoadingState;
    WResource::MemoryUsage m_Memory;
    WString m_sResourceDescription;
  };

  bool m_bShowDeleted;
  WString m_sTypeFilter;
  WString m_sNameFilter;
  WTime m_LastTableUpdate;
  bool m_bUpdateTable;

  bool m_bUpdateTypeBox;
  WSet<WString> m_ResourceTypes;
  WHashTable<WUInt64, ResourceData> m_Resources;
};
