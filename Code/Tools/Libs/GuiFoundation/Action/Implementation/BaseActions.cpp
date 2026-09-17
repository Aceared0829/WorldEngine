#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/BaseActions.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WNamedAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCategoryAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDynamicActionAndMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEnumerationMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WButtonAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WDynamicActionAndMenuAction::WDynamicActionAndMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
  m_bEnabled = true;
  m_bVisible = true;
}

WEnumerationMenuAction::WEnumerationMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
  : WDynamicMenuAction(context, szName, szIconPath)
{
  m_pEnumerationType = nullptr;
}

void WEnumerationMenuAction::InitEnumerationType(const WRTTI* pEnumerationType)
{
  m_pEnumerationType = pEnumerationType;
}

void WEnumerationMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();
  out_entries.Reserve(m_pEnumerationType->GetProperties().GetCount() - 1);
  WInt64 iCurrentValue = WReflectionUtils::MakeEnumerationValid(m_pEnumerationType, GetValue());

  // sort entries by group / category
  // categories appear in the order in which they are used on the reflected properties
  // within each category, items are sorted by 'order'
  // all items that have the same 'order' are sorted alphabetically by display string

  WStringBuilder sCurGroup;
  float fPrevOrder = -1;
  struct ItemWithOrder
  {
    float m_fOrder = -1;
    WDynamicMenuAction::Item m_Item;

    bool operator<(const ItemWithOrder& rhs) const
    {
      if (m_fOrder == rhs.m_fOrder)
      {
        return m_Item.m_sDisplay < rhs.m_Item.m_sDisplay;
      }

      return m_fOrder < rhs.m_fOrder;
    }
  };

  WTempHybridArray<ItemWithOrder, 16> unsortedItems;

  auto appendToOutput = [&]()
  {
    if (unsortedItems.IsEmpty())
      return;

    unsortedItems.Sort();

    if (!out_entries.IsEmpty())
    {
      // add a separator between groups
      out_entries.ExpandAndGetRef().m_ItemFlags.Add(WDynamicMenuAction::Item::ItemFlags::Separator);
    }

    for (const auto& sortedItem : unsortedItems)
    {
      out_entries.PushBack(sortedItem.m_Item);
    }

    unsortedItems.Clear();
  };

  for (auto pProp : m_pEnumerationType->GetProperties().GetSubArray(1))
  {
    if (pProp->GetCategory() == WPropertyCategory::Constant)
    {
      if (const WGroupAttribute* pGroup = pProp->GetAttributeByType<WGroupAttribute>())
      {
        if (sCurGroup != pGroup->GetGroup())
        {
          sCurGroup = pGroup->GetGroup();

          appendToOutput();
        }

        fPrevOrder = pGroup->GetOrder();
      }

      ItemWithOrder& newItem = unsortedItems.ExpandAndGetRef();
      newItem.m_fOrder = fPrevOrder;
      auto& item = newItem.m_Item;

      {
        WInt64 iValue = static_cast<const WAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<WInt64>();

        item.m_sDisplay = WTranslate(pProp->GetPropertyName());

        item.m_UserValue = iValue;
        if (m_pEnumerationType->IsDerivedFrom<WEnumBase>())
        {
          item.m_CheckState =
            (iCurrentValue == iValue) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
        }
        else if (m_pEnumerationType->IsDerivedFrom<WBitflagsBase>())
        {
          item.m_CheckState =
            ((iCurrentValue & iValue) != 0) ? WDynamicMenuAction::Item::CheckMark::Checked : WDynamicMenuAction::Item::CheckMark::Unchecked;
        }
      }
    }
  }

  appendToOutput();
}

WButtonAction::WButtonAction(const WActionContext& context, const char* szName, bool bCheckable, const char* szIconPath)
  : WNamedAction(context, szName, szIconPath)
{
  m_bCheckable = false;
  m_bChecked = false;
  m_bEnabled = true;
  m_bVisible = true;
}


WSliderAction::WSliderAction(const WActionContext& context, const char* szName)
  : WNamedAction(context, szName, nullptr)
{
  m_bEnabled = true;
  m_bVisible = true;
  m_iMinValue = 0;
  m_iMaxValue = 100;
  m_iCurValue = 50;
}

void WSliderAction::SetRange(WInt32 iMin, WInt32 iMax, bool bTriggerUpdate /*= true*/)
{
  W_ASSERT_DEBUG(iMin < iMax, "Invalid range");

  m_iMinValue = iMin;
  m_iMaxValue = iMax;

  if (bTriggerUpdate)
    TriggerUpdate();
}

void WSliderAction::SetValue(WInt32 iVal, bool bTriggerUpdate /*= true*/)
{
  m_iCurValue = WMath::Clamp(iVal, m_iMinValue, m_iMaxValue);
  if (bTriggerUpdate)
    TriggerUpdate();
}
