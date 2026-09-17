#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_ReflectionWidget.h>
#include <ads/DockWidget.h>

class WQtReflectionWidget : public ads::CDockWidget, public Ui_ReflectionWidget
{
public:
  Q_OBJECT

public:
  WQtReflectionWidget(ads::CDockManager* pDockManager, QWidget* pParent = 0);

  static WQtReflectionWidget* s_pWidget;

private Q_SLOTS:

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct PropertyData
  {
    WString m_sType;
    WString m_sPropertyName;
    WInt8 m_iCategory;
  };

  struct TypeData
  {
    TypeData() { m_pTreeItem = nullptr; }

    QTreeWidgetItem* m_pTreeItem;

    WUInt32 m_uiSize;
    WString m_sParentType;
    WString m_sPlugin;

    WHybridArray<PropertyData, 16> m_Properties;
  };

  bool UpdateTree();

  WMap<WString, TypeData> m_Types;
};
