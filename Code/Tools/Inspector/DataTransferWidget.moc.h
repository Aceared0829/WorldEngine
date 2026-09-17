#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_DataTransferWidget.h>
#include <ads/DockWidget.h>

class WQtDataWidget : public ads::CDockWidget, public Ui_DataTransferWidget
{
public:
  Q_OBJECT

public:
  WQtDataWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtDataWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonRefresh_clicked();
  virtual void on_ComboTransfers_currentIndexChanged(int index);
  virtual void on_ComboItems_currentIndexChanged(int index);
  virtual void on_ButtonSave_clicked();
  virtual void on_ButtonOpen_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    WString m_sMimeType;
    WString m_sExtension;
    WContiguousMemoryStreamStorage m_Storage;
    WString m_sFileName;
  };

  struct TransferData
  {
    WMap<WString, TransferDataObject> m_Items;
  };

  bool SaveToFile(TransferDataObject& item, WStringView sFile);

  TransferDataObject* GetCurrentItem();
  TransferData* GetCurrentTransfer();

  WMap<WString, TransferData> m_Transfers;
};
