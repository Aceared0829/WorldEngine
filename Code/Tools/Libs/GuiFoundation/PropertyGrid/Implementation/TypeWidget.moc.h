#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

class QGridLayout;
class WDocument;
class WQtManipulatorLabel;
struct WManipulatorManagerEvent;
class WObjectAccessorBase;

class W_GUIFOUNDATION_DLL WQtTypeWidget : public QWidget
{
  Q_OBJECT
public:
  WQtTypeWidget(QWidget* pParent, WQtPropertyGridWidget* pGrid, WObjectAccessorBase* pObjectAccessor, const WRTTI* pType,
    const char* szIncludeProperties, const char* szExcludeProperties);
  ~WQtTypeWidget();
  void SetSelection(const WArrayPtr<WPropertySelection>& items);
  const WHybridArray<WPropertySelection, 8>& GetSelection() const { return m_Items; }
  const WRTTI* GetType() const { return m_pType; }
  void PrepareToDie();

private:
  struct PropertyGroup
  {
    PropertyGroup(const WGroupAttribute* pAttr, float& ref_fOrder)
    {
      if (pAttr)
      {
        m_sGroup = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        m_fOrder = pAttr->GetOrder();
        if (m_fOrder == -1.0f)
        {
          ref_fOrder += 1.0f;
          m_fOrder = ref_fOrder;
        }
      }
      else
      {
        ref_fOrder += 1.0f;
        m_fOrder = ref_fOrder;
      }
    }

    void MergeGroup(const WGroupAttribute* pAttr)
    {
      if (pAttr)
      {
        m_sGroup = pAttr->GetGroup();
        m_sIconName = pAttr->GetIconName();
        if (pAttr->GetOrder() != -1.0f)
        {
          m_fOrder = pAttr->GetOrder();
        }
      }
    }

    bool operator==(const PropertyGroup& rhs) { return m_sGroup == rhs.m_sGroup; }
    bool operator<(const PropertyGroup& rhs) { return m_fOrder < rhs.m_fOrder; }

    WString m_sGroup;
    WString m_sIconName;
    float m_fOrder = -1.0f;
    WHybridArray<const WAbstractProperty*, 8> m_Properties;
  };

  void BuildUI(const WRTTI* pType, const char* szIncludeProperties, const char* szExcludeProperties);
  void BuildUI(const WRTTI* pType, const WMap<WString, const WManipulatorAttribute*>& manipulatorMap,
    const WDynamicArray<WUniquePtr<PropertyGroup>>& groups, const char* szIncludeProperties, const char* szExcludeProperties);

  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void ManipulatorManagerEventHandler(const WManipulatorManagerEvent& e);

  void UpdateProperty(const WDocumentObject* pObject, const WString& sProperty);
  void FlushQueuedChanges();
  void UpdatePropertyMetaState();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  bool m_bUndead = false;
  WQtPropertyGridWidget* m_pGrid = nullptr;
  WObjectAccessorBase* m_pObjectAccessor = nullptr;
  const WRTTI* m_pType = nullptr;
  WHybridArray<WPropertySelection, 8> m_Items;

  struct PropertyWidgetData
  {
    WQtPropertyWidget* m_pWidget;
    WQtManipulatorLabel* m_pLabel;
    WString m_sOriginalLabelText;
  };

  QGridLayout* m_pLayout;
  WMap<WString, PropertyWidgetData> m_PropertyWidgets;
  WHybridArray<WString, 1> m_QueuedChanges;
  QPalette m_Pal;
};
