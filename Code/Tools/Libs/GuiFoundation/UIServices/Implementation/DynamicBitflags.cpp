#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicBitflags.h>

WMap<WString, WDynamicBitflags> WDynamicBitflags::s_DynamicBitflags;

void WDynamicBitflags::Clear()
{
  m_ValidValues.Clear();
}

void WDynamicBitflags::SetValueAndName(WUInt32 uiBitPos, WStringView sName)
{
  W_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  auto it = m_ValidValues.FindOrAdd(W_BIT(uiBitPos));
  it.Value() = sName;
}

void WDynamicBitflags::RemoveValue(WUInt32 uiBitPos)
{
  W_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  m_ValidValues.Remove(W_BIT(uiBitPos));
}

bool WDynamicBitflags::IsValueValid(WUInt32 uiBitPos) const
{
  return m_ValidValues.Find(W_BIT(uiBitPos)).IsValid();
}

bool WDynamicBitflags::TryGetValueName(WUInt32 uiBitPos, WStringView& out_sName) const
{
  auto it = m_ValidValues.Find(W_BIT(uiBitPos));
  if (it.IsValid())
  {
    out_sName = it.Value();
    return true;
  }
  return false;
}

WDynamicBitflags& WDynamicBitflags::GetDynamicBitflags(WStringView sName)
{
  return s_DynamicBitflags[sName];
}
