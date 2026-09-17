#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Inspector/ui_MainWidget.h>
#include <QMainWindow>
#include <ads/DockManager.h>

class QTreeWidgetItem;

class WQtMainWidget : public ads::CDockWidget, public Ui_MainWidget
{
  Q_OBJECT
public:
  static WQtMainWidget* s_pWidget;

  WQtMainWidget(ads::CDockManager* pDockManager, QWidget* pParent = nullptr);
  ~WQtMainWidget();

  void ResetStats();
  void UpdateStats();
  virtual void closeEvent(QCloseEvent* pEvent) override;

  static void ProcessTelemetry(void* pUnuseed);

public Q_SLOTS:
  void ShowStatIn(bool);

private Q_SLOTS:
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:
  void SaveFavorites();
  void LoadFavorites();

  QTreeWidgetItem* CreateStat(WStringView sPath, bool bParent);
  void SetFavorite(const WString& sStat, bool bFavorite);

  WUInt32 m_uiMaxStatSamples;
  WTime m_MaxGlobalTime;

  struct StatSample
  {
    WTime m_AtGlobalTime;
    double m_Value;
  };

  struct StatData
  {
    WDeque<StatSample> m_History;

    WVariant m_Value;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavorite;

    StatData()
    {
      m_pItem = nullptr;
      m_pItemFavorite = nullptr;
    }
  };

  friend class WQtStatVisWidget;
  WMap<WString, StatData> m_Stats;
  WSet<WString> m_Favorites;
};
