#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicEnums.h>

WMap<WString, WDynamicEnum> WDynamicEnum::s_DynamicEnums;

void WDynamicEnum::Clear()
{
  m_ValidValues.Clear();
}

void WDynamicEnum::SetValueAndName(WInt32 iValue, WStringView sNewName)
{
  m_ValidValues[iValue] = sNewName;
}

void WDynamicEnum::RemoveValue(WInt32 iValue)
{
  m_ValidValues.Remove(iValue);
}

bool WDynamicEnum::IsValueValid(WInt32 iValue) const
{
  return m_ValidValues.Find(iValue).IsValid();
}

WStringView WDynamicEnum::GetValueName(WInt32 iValue) const
{
  auto it = m_ValidValues.Find(iValue);

  if (!it.IsValid())
    return "<invalid value>";

  return it.Value();
}

void WDynamicEnum::SetEditCommand(WStringView sCmd, const WVariant& value)
{
  m_sEditCommand = sCmd;
  m_EditCommandValue = value;
}

WDynamicEnum& WDynamicEnum::GetDynamicEnum(const char* szEnumName)
{
  return s_DynamicEnums[szEnumName];
}
